#include "server.h"

#include <errno.h>

#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include "network_session.h"
#include "session.h"

// TODO: use accept4
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

    LOG_INFO("Client %d successfully connected", socket);
  }

  return 0;
}

static void server_create_game_session(Server* server, NetworkSession* owner, CreateGameSessionMsg* message) {
  LOG_INFO("Creating session with password %s", message->pw);
  // TODO
}

static void server_connect_to_session(Server* server, NetworkSession* guest, ConnectToSessionMsg* message) {
  // TODO
}

static int server_read(Server* server, NetworkSession* session) {
  while (true) {
    int n = sizeof(session->input) - session->received;
    if (n == 0) {
      LOG_ERROR("Client %d overflown the buffer capacity", session->socket);
      return -1;
    }

    // TODO: flags
    int read = recv(session->socket, session->input, n, 0);
    if (read == -1) {
      if (errno == EWOULDBLOCK) {
        break;
      }

      LOG_WARN("Failed to receive data from socket: %s", strerror(errno));
      return -1;
    }

    if (read == 0) {
      LOG_INFO("Client %d disconnected", session->socket);
      return -1;
    }

    session->received += read;
    if (session->game == NULL) {
      // attempt to parse CreateSession or JoinSession message
      ClientMsg message;
      int n = parse_client_message(&message, &session->input[0], session->received);
      if (n < 0) {
        return n;
      }

      if (n > 0) {
        // TODO: use ring buffer
        memmove(session->input, session->input + n, session->received - n);

        switch (message.id) {
          case CREATE_GAME_SESSION:
            server_create_game_session(server, session, &message.create_game_session);
            break;
          case CONNECT_TO_SESSION:
            server_connect_to_session(server, session, &message.connect_to_session);
            break;
          default:
            LOG_WARN("Unexpected message type received: %d", message.id);
            return -1;
        }
      }

      continue;
    }

    // process game logic
    // TODO
  }

  return 0;
}

static int server_write(Server* server, NetworkSession* session) {
  // TODO
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

// Disconnect session without notifying the client
static void server_disconnect(Server* server, NetworkSession* session) {
  if (epoll_ctl(server->poll, EPOLL_CTL_DEL, session->socket, NULL) == -1) {
    LOG_ERROR("Failed to remove socket from epoll: %s", strerror(errno));
  }
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
