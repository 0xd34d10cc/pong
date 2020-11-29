#include "server.h"

#include <errno.h>
#include <string.h>
#include <stdalign.h>

#include <arpa/inet.h>

#include "log.h"
#include "bool.h"


static int connection_id(Connection* connection) {
  return connection->stream.state.fd;
}

int server_init(Server* server, const char* ip, unsigned short port) {
  if (reactor_init(&server->reactor) == -1) {
    LOG_ERROR("Failed to initialize reactor: %s", strerror(errno));
    return -1;
  }

  if (tcp_listener_init(&server->listener, &server->reactor, ip, port) == -1) {
    LOG_ERROR("Failed to initialize tcp listener: %s", strerror(errno));
    return -1;
  }

  pool_init(
    &server->connections,
    server->connections_memory, sizeof(server->connections_memory),
    sizeof(Connection), alignof(Connection)
  );
  pool_init(
    &server->lobbies,
    server->lobbies_memory, sizeof(server->lobbies_memory),
    sizeof(Lobby), alignof(Lobby)
  );
  return 0;
}

void server_close(Server* server) {
  tcp_listener_close(&server->listener);

  // TODO: close active connections

  reactor_close(&server->reactor);
}

static int server_accept(Server* server) {
  struct sockaddr_in addr;
  socklen_t addr_size = sizeof(addr);
  while (true) {
    Connection* connection = pool_aquire(&server->connections);
    if (connection == NULL) {
      LOG_WARN("Could not accept connection: the connection pool is full");
      if (tcp_listener_stop_accept(&server->listener) == -1) {
        LOG_ERROR("Failed to pause accept() operation: %s", strerror(errno));
        return -1;
      }
      return 0;
    }

    int n = tcp_listener_accept(&server->listener, &connection->stream, &connection->address);
    if (n <= 0) {
      pool_release(&server->connections, connection);
      return n;
    }

    if (tcp_start_recv(&connection->stream) == -1) {
      LOG_WARN("Failed to start read opertion on accepted socket: %s", strerror(errno));
      tcp_close(&connection->stream);
      pool_release(&server->connections, connection);
      return -1;
    }

    connection->lobby = NULL;
    LOG_INFO("[%02d] Client successfully connected", connection_id(connection));
  }
}

static int server_send_message(Server* server, Connection* connection, ServerMessage* message) {
  char buffer[MAX_MESSAGE_SIZE];
  int n = server_message_write(message, buffer, sizeof(buffer));
  if (n == 0) {
    LOG_WARN("[%02d] Failed to serialize message", connection_id(connection));
    return -1;
  }

  n = tcp_start_send(&connection->stream, buffer, n);
  if (n == 0) {
    LOG_WARN("[%02d] Failed to send message: output buffer is at capacity", connection_id(connection));
    return -1;
  }

  if (n == -1) {
    LOG_WARN("[%02d] Failed to send message: %s", strerror(errno));
    return -1;
  }

  return 0;
}

static int server_send_error(Server* server, Connection* connection, int error) {
  ServerMessage message;
  message.id = ERROR_STATUS;
  message.error.status = error;
  return server_send_message(server, connection, &message);
}

static void lobby_init(Lobby* lobby, Connection* owner, const char* password) {
  lobby->owner = owner;
  lobby->guest = NULL;
  strcpy(lobby->password, password);
  // FIXME: unhardcode the board size
  game_init(&lobby->game, 800, 600);
}

static int server_create_lobby(Server* server, Connection* owner, CreateLobby* message) {
  if (owner->lobby != NULL) {
    int lobby_id = pool_index(&server->lobbies, owner->lobby);
    LOG_INFO("[%02d] Failed to create game lobby: client already in lobby #%d", connection_id(owner), lobby_id);
    // TODO: disconnect from current lobby and create a new one instead
    return server_send_error(server, owner, INTERNAL_ERROR);
  }

  Lobby* lobby = pool_aquire(&server->lobbies);
  if (lobby == NULL) {
    LOG_ERROR("[%02d] Failed to create new lobby: out of memory", connection_id(owner));
    return server_send_error(server, owner, INTERNAL_ERROR);
  }

  lobby_init(lobby, owner, message->password);
  owner->lobby = lobby;

  int lobby_id = pool_index(&server->lobbies, lobby);
  LOG_INFO("[%02d] Created lobby #%d with password \"%s\"", connection_id(owner), lobby_id, lobby->password);

  ServerMessage response;
  response.id = LOBBY_CREATED;
  response.lobby_created.id = lobby_id;
  return server_send_message(server, owner, &response);
}

static int server_join_lobby(Server* server, Connection* guest, JoinLobby* message) {
  int lobby_id = message->id;
  Lobby* lobby = pool_at(&server->lobbies, lobby_id);
  if (!pool_contains(&server->lobbies, lobby)) {
    LOG_WARN("[%02d] Tried to join to invalid lobby #%d", connection_id(guest), lobby_id);
    return server_send_error(server, guest, INVALID_LOBBY_ID);
  }

  if (lobby->guest != NULL || lobby->owner == guest) {
    LOG_WARN("[%02d] Failed to join lobby #%d: lobby is full", connection_id(guest), lobby_id);
    return server_send_error(server, guest, LOBBY_IS_FULL);
  }

  if (strcmp(lobby->password, message->password)) {
    LOG_WARN("[%02d] Failed to join lobby #%d: invalid password: %s", connection_id(guest), lobby_id, message->password);
    return server_send_error(server, guest, INVALID_PASSWORD);
  }

  lobby->guest = guest;
  guest->lobby = lobby;

  Connection* owner = lobby->owner;

  ServerMessage response;
  response.id = LOBBY_JOINED;
  strcpy(response.lobby_joined.ipv4, inet_ntoa(owner->address.sin_addr));
  if (server_send_message(server, guest, &response) < 0) {
    return -1;
  }

  strcpy(response.lobby_joined.ipv4, inet_ntoa(guest->address.sin_addr));
  LOG_INFO("[%02d] Joined lobby #%d", connection_id(guest), lobby_id);
  return server_send_message(server, owner, &response);
}

static int server_process_message(Server* server, Connection* connection, ClientMessage* message) {
  int status = 0;
  switch (message->id) {
    case CREATE_LOBBY:
      status = server_create_lobby(server, connection, &message->create_lobby);
      break;
    case JOIN_LOBBY:
      status = server_join_lobby(server, connection, &message->join_lobby);
      break;
    default:
      LOG_WARN("[%02d] Unexpected message: %d", connection_id(connection), message->id);
      status = -1;
      break;
  }

  return status;
}

static int server_read(Server* server, Connection* connection) {
  while (true) {
    int n = tcp_recv(&connection->stream);
    if (n == 0) {
      return -1;
    }

    if (n < 0) {
      LOG_WARN("[%02d] Read operation failed: %s", connection_id(connection), strerror(errno));
      return -1;
    }

    // parse messages
    int total = 0;
    while (true) {
      ClientMessage message;
      int n = client_message_read(&message, connection->stream.input + total, connection->stream.received - total);
      if (n < 0) {
        LOG_WARN("[%02d] Client sent invalid message", connection_id(connection));
        return -1;
      }

      if (n == 0) {
        break;
      }

      if (server_process_message(server, connection, &message) == -1) {
        return -1;
      }

      total += n;
    }

    bool more = connection->stream.received == sizeof(connection->stream.input);
    tcp_consume(&connection->stream, total);

    if (!more) {
      break;
    }
  }

  return 0;
}

static int server_event(Server* server, Connection* connection, unsigned int event) {
  if (event & IO_EVENT_READ) {
    if (server_read(server, connection) < 0) {
      return -1;
    }
  }

  if (event & IO_EVENT_WRITE) {
    if (tcp_send(&connection->stream) < 0) {
      return -1;
    }
  }

  return 0;
}

static void server_disconnect(Server* server, Connection* connection) {
  if (connection->lobby) {
    int lobby_id = pool_index(&server->lobbies, connection->lobby);

    Connection* opponent = NULL;
    if (connection == connection->lobby->owner) {
      opponent = connection->lobby->guest;
    }
    else if (connection == connection->lobby->guest) {
      opponent = connection->lobby->owner;
    }
    else {
      LOG_ERROR("[%02d] Inconsistent state: player is in game lobby #%d, but he isn't one of the players",
                connection_id(connection), lobby_id);
    }

    if (opponent) {
      opponent->lobby = NULL;
      if (server_send_error(server, opponent, OPPONENT_DISCONNECTED) < 0) {
        LOG_WARN("[%02d] Failed to notify about opponent disconnection");
        server_disconnect(server, opponent);
      }
    }

    LOG_INFO("Lobby #%d closed", lobby_id);
    pool_release(&server->lobbies, connection->lobby);
  }

  LOG_INFO("[%02d] Disconnected", connection_id(connection));
  tcp_close(&connection->stream);
  bool was_full = pool_size(&server->connections) == pool_capacity(&server->connections);
  pool_release(&server->connections, connection);

  if (was_full) {
    if (tcp_listener_start_accept(&server->listener) == -1) {
      LOG_ERROR("Failed to resume accept() operation: %s", strerror(errno));
    }
    LOG_INFO("Connection pool is no longer full. Starting to accept new clients.");
  }
}

static const int MAX_EVENTS = 64;

int server_run(Server* server) {
  if (tcp_listener_start_accept(&server->listener) == -1) {
    LOG_ERROR("Failed to start accept() operatioon: %s", strerror(errno));
    return -1;
  }

  IOEvent events[MAX_EVENTS];
  while (true) {
    int n_events = reactor_poll(&server->reactor, events, MAX_EVENTS, -1);
    if (n_events == -1) {
      LOG_ERROR("reactor_poll() failed: %s", strerror(errno));
      return -1;
    }

    for (int i = 0; i < n_events;  ++i) {
      Evented* object = events[i].object;
      if (object == &server->listener.state) {
        if (server_accept(server) < 0) {
          return -1;
        }
      }
      else {
        Connection* connection = (Connection*)object;
        if (server_event(server, connection, events[i].events) < 0) {
          server_disconnect(server, connection);
        }
      }
    }
  }
}
