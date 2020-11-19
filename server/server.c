#include "server.h"

#include <errno.h>
#include <string.h>

#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include "network_session.h"
#include "session.h"
#include "messages.h"
#include "log.h"
#include "bool.h"

// TODO: use accept4
// TOOD: move this function to some common part
static void set_nonblocking(int socket) {
  int flags = fcntl(socket, F_GETFL, 0);
  if (flags == -1) {
    LOG_ERROR("Can't get flags from socket");
    return;
  }

  flags |= O_NONBLOCK;
  if (fcntl(socket, F_SETFL, flags) != 0) {
    LOG_ERROR("Can't set flags for socket");
  }
}

int server_init(Server* server, const char* ip, unsigned short port) {
  struct sockaddr_in addr = { 0 };
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(ip);
  int master = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (master == -1) {
    LOG_ERROR("Failed to create master socket: %s", strerror(errno));
    return -1;
  }

  set_nonblocking(master);
  int flag = 1;
  if (setsockopt(master, SOL_SOCKET, SO_REUSEADDR, (const void*)&flag, sizeof(int)) == -1) {
    LOG_WARN("Failed to set SO_RESUEADDR socket option: %s", strerror(errno));
  }

  if (bind(master, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
    LOG_ERROR("Failed to bind to the address: %s", strerror(errno));
    close(master);
    return -1;
  }

  if (listen(master, 20) == -1) {
    LOG_ERROR("Call to listen() failed: %s", strerror(errno));
    close(master);
    return -1;
  }

  // NOTE: since Linux 2.6.8 size argument is ignored
  // TODO: set EPOLLET
  int poll = epoll_create(1);
  if (poll == -1) {
    LOG_ERROR("Failed to create epoll instance: %s", strerror(errno));
    close(master);
    return -1;
  }

  server->poll = poll;
  server->master_socket = master;
  pool_init(&server->connections, sizeof(NetworkSession));
  pool_init(&server->sessions, sizeof(Session));

  return 0;
}

void server_close(Server* server) {
  close(server->master_socket);

  // TODO: close each active network session

  close(server->poll);
}

static int server_accept(Server* server) {
  struct sockaddr_in addr;
  socklen_t addr_size = sizeof(addr);
  while (true) {
    int socket = accept(server->master_socket, (struct sockaddr*)&addr, &addr_size);
    if (socket == -1) {
      if (errno == EWOULDBLOCK) {
        break;
      }

      LOG_ERROR("accept() failed: %s", strerror(errno));
      return -1;
    }

    set_nonblocking(socket);

    NetworkSession* session = pool_aquire(&server->connections);
    if (session == NULL) {
      LOG_WARN("Could not accept connection: the connection pool is full");
      close(socket);
      return 0;
    }

    network_session_init(session, socket, &addr);

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = session;
    if (epoll_ctl(server->poll, EPOLL_CTL_ADD, socket, &event) == -1) {
      LOG_ERROR("Failed to add client socket to epoll: %s", strerror(errno));
      return -1;
    }
    session->events |= IO_EVENT_READ;

    LOG_INFO("[%02d] Client successfully connected", socket);
  }

  return 0;
}

static int server_register(Server* server, NetworkSession* session, unsigned events) {
  if (session->events == events) {
    return 0;
  }

  session->events = events;
  unsigned poll_events = 0;
  if (session->events &  IO_EVENT_READ) {
    poll_events |= EPOLLIN;
  }

  if (session->events & IO_EVENT_WRITE) {
    poll_events |= EPOLLOUT;
  }

  struct epoll_event event;
  event.events = poll_events;
  event.data.ptr = session;
  if (epoll_ctl(server->poll, EPOLL_CTL_MOD, session->socket, &event) == -1) {
    LOG_ERROR("[%02d] Failed to modify event subscription: %s", session->socket, strerror(errno));
    return -1;
  }

  return 0;
}

static int server_write(Server* server, NetworkSession* session) {
  int total = 0;
  while (total != session->to_send) {
    // TODO: flags
    int sent = send(session->socket, session->output + total, session->to_send - total, 0);
    if (sent == -1) {
      if (errno == EWOULDBLOCK) {
        break;
      }

      LOG_WARN("[%02d] send() failed: %s", session->socket, strerror(errno));
      return -1;
    }

    total += sent;
  }

  memmove(session->output, session->output + total, session->to_send - total);
  session->to_send -= total;

  if (session->to_send == 0) {
    // no more data to send, unregister from write events
    return server_register(server, session, IO_EVENT_READ);
  }

  return 0;
}

static int server_send_message(Server* server, NetworkSession* session, ServerMessage* message) {
  int n = server_message_write(message, session->output + session->to_send, sizeof(session->output) - session->to_send);
  if (n == 0) {
    LOG_WARN("[%02d] Failed to send message: outbut buffer is at capacity", session->socket);
    return -1;
  }

  session->to_send += n;
  return server_register(server, session, IO_EVENT_READ | IO_EVENT_WRITE);
}

static int server_send_error(Server* server, NetworkSession* session, int error) {
  ServerMessage message;
  message.id = ERROR_STATUS;
  message.error_status.status = error;
  return server_send_message(server, session, &message);
}

static int server_create_session(Server* server, NetworkSession* owner, CreateSession* message) {
  if (owner->game != NULL) {
    int session_id = pool_index(&server->sessions, owner->game);
    LOG_INFO("[%02d] Failed to create game session: client already in session #%d", owner->socket, session_id);
    // TODO: disconnect from current session and create a new one instead
    return server_send_error(server, owner, INTERNAL_ERROR);
  }

  Session* session = pool_aquire(&server->sessions);
  if (session == NULL) {
    LOG_ERROR("[%02d] Failed to create new session: session pool is at capacity", owner->socket);
    return server_send_error(server, owner, INTERNAL_ERROR);
  }

  session_init(session, owner, message->password);
  owner->game = session;

  int session_id = pool_index(&server->sessions, session);
  LOG_INFO("[%02d] Created session #%d with password \"%s\"", owner->socket, session_id, message->password);

  ServerMessage response;
  response.id = SESSION_CREATED;
  response.session_created.session_id = session_id;
  return server_send_message(server, owner, &response);
}

static int server_join_session(Server* server, NetworkSession* guest, JoinSession* message) {
  int session_id = message->session_id;
  Session* session = pool_at(&server->sessions, session_id);
  if (session == NULL) {
    LOG_WARN("[%02d] Tried to join to invalid session #%d", guest->socket, session_id);
    return server_send_error(server, guest, INVALID_SESSION_ID);
  }

  if (session->player2 != NULL || session->player1 == guest) {
    LOG_WARN("[%02d] Failed to join session #%d: lobby is full", guest->socket, session_id);
    return server_send_error(server, guest, SESSION_IS_FULL);
  }

  if (strcmp(session->password, message->password)) {
    LOG_WARN("[%02d] Failed to join session #%d: invalid password", guest->socket, session_id);
    return server_send_error(server, guest, INVALID_PASSWORD);
  }

  session->player2 = guest;
  guest->game = session;

  NetworkSession* owner = session->player1;

  ServerMessage response;
  response.id = SESSION_JOINED;
  strcpy(response.session_joined.ipv4, inet_ntoa(owner->address.sin_addr));
  if (server_send_message(server, guest, &response) < 0) {
    return -1;
  }

  strcpy(response.session_joined.ipv4, inet_ntoa(guest->address.sin_addr));
  LOG_INFO("[%02d] Joined session #%d", guest->socket, session_id);
  return server_send_message(server, owner, &response);
}

static int server_process_message(Server* server, NetworkSession* session, ClientMessage* message) {
  int status = 0;
  switch (message->id) {
    case CREATE_SESSION:
      status = server_create_session(server, session, &message->create_session);
      break;
    case JOIN_SESSION:
      status = server_join_session(server, session, &message->join_session);
      break;
    default:
      LOG_WARN("[%02d] Unexpected message: %d", session->socket, message->id);
      status = -1;
      break;
  }

  return status;
}

static int server_read(Server* server, NetworkSession* session) {
  while (true) {
    int n = sizeof(session->input) - session->received;
    if (n == 0) {
      LOG_ERROR("[%02d] Input buffer is out of capacty", session->socket);
      return -1;
    }

    // TODO: flags
    int read = recv(session->socket, session->input, n, 0);
    if (read == -1) {
      if (errno == EWOULDBLOCK) {
        break;
      }

      LOG_WARN("[%02d] recv() failed: %s", session->socket, strerror(errno));
      return -1;
    }

    if (read == 0) {
      return -1;
    }

    session->received += read;

    // parse messages
    while (true) {
      ClientMessage message;
      int n = client_message_read(&message, &session->input[0], session->received);
      if (n < 0) {
        return n;
      }

      if (n == 0) {
        break;
      }

      if (server_process_message(server, session, &message) == -1) {
        return -1;
      }

      memmove(session->input, session->input + n, session->received - n);
      session->received -= n;
    }
  }

  return 0;
}


static int server_event(Server* server, NetworkSession* session, unsigned int event) {
  if (event & EPOLLIN) {
    if (server_read(server, session) < 0) {
      return -1;
    }
  }

  if (event & EPOLLOUT) {
    if (server_write(server, session) < 0) {
      return -1;
    }
  }

  return 0;
}

static void server_disconnect(Server* server, NetworkSession* session) {
  if (epoll_ctl(server->poll, EPOLL_CTL_DEL, session->socket, NULL) == -1) {
    LOG_ERROR("Failed to remove socket from epoll: %s", strerror(errno));
  }

  if (session->game) {
    int session_id = pool_index(&server->sessions, session->game);

    NetworkSession* opponent = NULL;
    if (session == session->game->player1) {
      opponent = session->game->player2;
    }
    else if (session == session->game->player2) {
      opponent = session->game->player1;
    }
    else {
      LOG_ERROR("[%02d] Inconsistent state: player is in game session #%d, but he isn't one of the players",
                session->socket, session_id);
    }

    if (opponent) {
      opponent->game = NULL;
      if (server_send_error(server, opponent, OPPONENT_DISCONNECTED) < 0) {
        server_disconnect(server, opponent);
      }
    }

    LOG_INFO("Session #%d closed", session_id);
    session_close(session->game);
    pool_release(&server->sessions, session->game);
  }

  LOG_INFO("[%02d] Disconnected", session->socket);
  network_session_close(session);
  pool_release(&server->connections, session);
}

static const int MAX_EVENTS = 64;

int server_run(Server* server) {
  struct epoll_event event;
  event.events = EPOLLIN;
  event.data.ptr = NULL;
  if (epoll_ctl(server->poll, EPOLL_CTL_ADD, server->master_socket, &event) == -1) {
    LOG_ERROR("Failed to add master socket to epoll: %s", strerror(errno));
    return -1;
  }

  struct epoll_event events[MAX_EVENTS];
  while (true) {
    int n_events = epoll_wait(server->poll, events, MAX_EVENTS, -1);
    if (n_events == -1) {
      LOG_ERROR("epoll_wait() failed: %s", strerror(errno));
      return -1;
    }

    for (int i = 0; i < n_events;  ++i) {
      NetworkSession* session = events[i].data.ptr;
      if (session == NULL) {
        if (server_accept(server) < 0) {
          return -1;
        }
      }
      else {
        if (server_event(server, session, events[i].events) < 0) {
          server_disconnect(server, session);
        }
      }
    }
  }
}
