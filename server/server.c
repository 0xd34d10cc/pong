#include "server.h"

#include <errno.h>
#include <string.h>
#include <stdalign.h>

#include <arpa/inet.h>
#include <sys/timerfd.h>

#include "log.h"
#include "bool.h"


static int connection_id(Connection* connection) {
  return connection->stream.state.fd;
}

int server_init(Server* server, const char* ip, unsigned short port) {
  atomic_store(&server->running, false);

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

  int timer = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

  server->timer = (Evented){.fd = timer, .events = 0};
  LOG_INFO("Max connections: %d", pool_capacity(&server->connections));
  LOG_INFO("Max lobbies:     %d", pool_capacity(&server->lobbies));
  return 0;
}

static int server_accept(Server* server) {
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

static int send_message(Connection* connection, ServerMessage* message) {
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

static int send_error(Connection* connection, int error) {
  ServerMessage message;
  message.id = ERROR_STATUS;
  message.error.status = error;
  return send_message(connection, &message);
}

static void lobby_init(Lobby* lobby, Connection* owner, const char* password) {
  lobby->owner = owner;
  lobby->guest = NULL;
  strcpy(lobby->password, password);
  // FIXME: unhardcode the board size
  game_init(&lobby->game, true);
}

static int server_create_lobby(Server* server, Connection* owner, CreateLobby* message) {
  if (owner->lobby != NULL) {
    int lobby_id = pool_index(&server->lobbies, owner->lobby);
    LOG_INFO("[%02d] Failed to create game lobby: client already in lobby #%d", connection_id(owner), lobby_id);
    // TODO: disconnect from current lobby and create a new one instead
    return send_error(owner, INTERNAL_ERROR);
  }

  Lobby* lobby = pool_aquire(&server->lobbies);
  if (lobby == NULL) {
    LOG_ERROR("[%02d] Failed to create new lobby: out of memory", connection_id(owner));
    return send_error(owner, INTERNAL_ERROR);
  }

  lobby_init(lobby, owner, message->password);
  owner->lobby = lobby;

  int lobby_id = pool_index(&server->lobbies, lobby);
  LOG_INFO("[%02d] Created lobby #%d with password \"%s\"", connection_id(owner), lobby_id, lobby->password);

  ServerMessage response;
  response.id = LOBBY_CREATED;
  response.lobby_created.id = lobby_id;
  return send_message(owner, &response);
}

static int process_active_lobby(Lobby* lobby, int id) {
  if (!lobby->owner || !lobby->guest) {
    return 0;
  }

  if (lobby->game.state != STATE_RUNNING) {
    return 0;
  }

  // TODO: get rid of 16 after game_step_end refactoring
  game_step_end(&lobby->game, 16);

  if (lobby->game.state == STATE_LOST || lobby->game.state == STATE_WON) {
    const char* state = lobby->game.state == STATE_LOST ? "lost" : "won";

    LOG_INFO("In lobby #%d owner has %s", id, state);
    ServerMessage msg;

    msg.id = GAME_STATE_UPDATE;
    msg.game_state_update.state = lobby->game.state;


    if (send_message(lobby->owner, &msg) < 0) {
      return -1;
    }

    msg.game_state_update.state = lobby->game.state == STATE_LOST ? STATE_WON : STATE_LOST;
    if (send_message(lobby->guest, &msg) < 0) {
      return -1;
    }

    return 0;
  }

  ServerMessage response;

  response.id = SERVER_UPDATE;

  response.server_update.ball_position.x = lobby->game.ball.position.x;
  response.server_update.ball_position.y = -1 * lobby->game.ball.position.y;

  response.server_update.opponent_position.x = lobby->game.player.position.x;
  response.server_update.opponent_position.y = -1 * lobby->game.player.position.y;

  if (send_message(lobby->guest, &response) < 0) {
    return -1;
  }

  response.server_update.ball_position.y = lobby->game.ball.position.y;
  response.server_update.opponent_position.x = lobby->game.opponent.position.x;
  response.server_update.opponent_position.y = lobby->game.opponent.position.y;

  if (send_message(lobby->owner, &response) < 0) {
    return -1;
  }

  return 0;
}

static int server_process_active_lobbies(Server* server) {
  for (Lobby* lobby = pool_first(&server->lobbies); lobby != NULL; lobby = pool_next(&server->lobbies, lobby)) {

    int lobby_id = pool_index(&server->lobbies, lobby);
    if (process_active_lobby(lobby, lobby_id) < 0) {
      LOG_WARN("Failed to update lobby with #%d", lobby_id);
    }
  }

  return 0;

}

static int server_join_lobby(Server* server, Connection* guest, JoinLobby* message) {
  int lobby_id = message->id;
  Lobby* lobby = pool_at(&server->lobbies, lobby_id);
  if (!pool_contains(&server->lobbies, lobby)) {
    LOG_WARN("[%02d] Tried to join to invalid lobby #%d", connection_id(guest), lobby_id);
    return send_error(guest, INVALID_LOBBY_ID);
  }

  if (lobby->guest != NULL || lobby->owner == guest) {
    LOG_WARN("[%02d] Failed to join lobby #%d: lobby is full", connection_id(guest), lobby_id);
    return send_error(guest, LOBBY_IS_FULL);
  }

  if (strcmp(lobby->password, message->password)) {
    LOG_WARN("[%02d] Failed to join lobby #%d: invalid password: %s", connection_id(guest), lobby_id, message->password);
    return send_error(guest, INVALID_PASSWORD);
  }

  lobby->guest = guest;
  guest->lobby = lobby;

  Connection* owner = lobby->owner;

  ServerMessage response;
  response.id = LOBBY_JOINED;
  strcpy(response.lobby_joined.ipv4, inet_ntoa(owner->address.sin_addr));
  if (send_message(guest, &response) < 0) {
    return -1;
  }

  strcpy(response.lobby_joined.ipv4, inet_ntoa(guest->address.sin_addr));
  LOG_INFO("[%02d] Joined lobby #%d", connection_id(guest), lobby_id);
  return send_message(owner, &response);
}


static int server_client_update(Server* server, Connection* player, ClientUpdate* message) {
  if (!pool_contains(&server->lobbies, player->lobby)) {
    LOG_WARN("[%02d] Failed to find lobby.", connection_id(player));
    return send_error(player, INVALID_LOBBY_ID);
  }

  if(player->lobby->owner == player) {
    player->lobby->game.player_speed = message->speed;
  }
  else {
    player->lobby->game.opponent_speed = message->speed;
  }

  return 0;
}

static int server_client_state_update(Server* server, Connection* player,
                                      ClientStateUpdate* message) {
  (void)server;
  switch (message->state) {
    case CLIENT_STATE_RESTART:
      if (player->lobby == NULL) {
        return send_error(player, NOT_IN_GAME);
      }

      if (!player->lobby->owner || !player->lobby->guest) {
        return send_error(player, NOT_IN_GAME);
      }

      if (player->lobby->game.state != STATE_RUNNING) {
        game_event(&player->lobby->game, EVENT_RESTART);
        ServerMessage server_msg;
        server_msg.id = GAME_STATE_UPDATE;
        server_msg.game_state_update.state = STATE_RUNNING;

        if (send_message(player->lobby->owner, &server_msg) < 0) {
          return -1;
        }

        if (send_message(player->lobby->guest, &server_msg) < 0) {
          return -1;
        }

      }
      break;
    default:
      LOG_ERROR("[%02d] Received wrong client state: %d", connection_id(player), message->state);
      return -1;
  }
  return 0;
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
    case CLIENT_UPDATE:
      status = server_client_update(server, connection, &message->client_update);
      break;
    case CLIENT_STATE_UPDATE:
      status = server_client_state_update(server, connection, &message->client_state_update);
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
      if (send_error(opponent, OPPONENT_DISCONNECTED) < 0) {
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
static const int POLL_INTERVAL_MS = 128;

int server_run(Server* server) {
  if (tcp_listener_start_accept(&server->listener) == -1) {
    LOG_ERROR("Failed to start accept() operatioon: %s", strerror(errno));
    return -1;
  }

  // TODO: wrap timer into something crossplatform and readable
  struct itimerspec time = {.it_value = {.tv_sec = 1, .tv_nsec = 0},
                            .it_interval = {.tv_sec = 0, .tv_nsec = 16 * 1000 * 1000}};

  if (timerfd_settime(server->timer.fd, 0, &time, NULL) < 0) {
    LOG_ERROR("Failed to set time for timer: %s", strerror(errno));
    return -1;
  }

  if (reactor_register(&server->reactor, &server->timer, IO_EVENT_READ) < 0) {
    LOG_ERROR("Failed to register the timer: %s", strerror(errno));
    return -1;
  }

  atomic_store(&server->running, true);

  IOEvent events[MAX_EVENTS];
  while (atomic_load(&server->running)) {
    int n_events = reactor_poll(&server->reactor, events, MAX_EVENTS, POLL_INTERVAL_MS);
    if (n_events == -1) {
      if (errno == EAGAIN || errno == EINTR) {
        continue;
      }

      LOG_ERROR("reactor_poll() failed: %s", strerror(errno));
      return -1;
    }

    for (int i = 0; i < n_events;  ++i) {
      Evented* object = events[i].object;
      if (object == &server->timer) {
        while (true) {
          uint64_t n_ticks = 0;

          int res = read(server->timer.fd, &n_ticks, sizeof(n_ticks));

          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
          }

          if (res != sizeof(n_ticks)) {
            LOG_ERROR("timer internal error: %s", strerror(errno));
            return -1;
          }

        }
        if (server_process_active_lobbies(server) < 0) {
          return -1;
        }
      }

      else if (object == &server->listener.state) {
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

  return 0;
}

void server_stop(Server* server) {
  atomic_store(&server->running, false);
}

void server_close(Server* server) {
  for (Connection* c = pool_first(&server->connections); c != NULL; c = pool_next(&server->connections, c)) {
    server_disconnect(server, c);
  }

  tcp_listener_close(&server->listener);
  reactor_close(&server->reactor);
}

