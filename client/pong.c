#include "pong.h"

#include "log.h"
#include "game/protocol.h"

#include <SDL2/SDL_video.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_timer.h>

#include <stdbool.h>

#define DEFAULT_WINDOW_WIDTH 800
#define DEFAULT_WINDOW_HEIGHT 600

#define RECONNECT_DELAY 1000 * 3

int pong_init(Pong* pong, Args* params) {
  pong->running = false;
  pong->window = SDL_CreateWindow(
      "pong",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      DEFAULT_WINDOW_WIDTH,
      DEFAULT_WINDOW_HEIGHT,
      SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
  );

  if (!pong->window) {
    LOG_ERROR("Failed to create window: %s", SDL_GetError());
    return -1;
  }

  if (renderer_init(&pong->renderer, pong->window)) {
    return -1;
  }
  if (reactor_init(&pong->reactor)) {
    LOG_ERROR("Can't initialize reactor");
    return -1;
  }

  pong->connection_state.state = DISCONNECTED;
  strcpy(pong->connection_state.ip, params->ip);
  pong->connection_state.port = params->port;

  tcp_init(&pong->tcp_stream, &pong->reactor);

  pong->game_session.id = params->lobby_id;
  strcpy(pong->game_session.password, params->password);
  // TODO: pass game session state instead of game mode
  switch (params->game_mode) {
    case LOCAL_GAME:
      pong->game_session.state = NOT_IN_LOBBY;
      pong->connection_state.state = LOCAL;
      break;

    case REMOTE_NEW_GAME:
      pong->game_session.state = NOT_IN_LOBBY;
      break;

    case REMOTE_CONNECT_GAME:
      pong->game_session.state = WANT_TO_JOIN;
      break;
  }

  pong->game_session.opponent_ip[0] = '\0';

  game_init(&pong->game, pong->connection_state.state != LOCAL);

  return 0;
}

void pong_close(Pong* pong) {
  renderer_close(&pong->renderer);
  SDL_DestroyWindow(pong->window);
}

static int prepare_and_send(Pong* pong, const ClientMessage* msg) {
  char buf[MAX_MESSAGE_SIZE];

  int n = client_message_write(msg, buf, sizeof(buf));

  if (n == 0) {
    return -1;
  }

  int send_res = tcp_start_send(&pong->tcp_stream, buf, n);
  if (send_res <= 0) {
    LOG_WARN("Send operation failed with code: %d, msg: %s", send_res, strerror(errno));
    return -1;
  }

  return 0;
}


static void pong_event(Pong* pong, SDL_Event* event) {
  switch (event->type) {
    case SDL_QUIT:
      pong->running = false;
      break;
    case SDL_KEYDOWN:
      switch (event->key.keysym.scancode) {
        case SDL_SCANCODE_Q:
          if (SDL_GetModState() & KMOD_LCTRL) {
            pong->running = false;
          }
          break;
        case SDL_SCANCODE_RETURN:
          if (pong->connection_state.state == LOCAL) {
            game_event(&pong->game, EVENT_RESTART);
          }
          else if (pong->connection_state.state == CONNECTED &&
                  pong->game_session.state == PLAYING) {
            ClientMessage msg;
            msg.id = CLIENT_STATE_UPDATE;
            msg.client_state_update.state = CLIENT_STATE_RESTART;

            if (prepare_and_send(pong, &msg) < 0) {
              LOG_ERROR("Can't send restart to the server");
            }
          }
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

static void pong_process_events(Pong* pong) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    pong_event(pong, &event);
  }

  const Uint8* kb = SDL_GetKeyboardState(NULL);
  if (kb[SDL_SCANCODE_LEFT] || kb[SDL_SCANCODE_A]) {
    game_event(&pong->game, EVENT_MOVE_LEFT);
  }
  else if (kb[SDL_SCANCODE_RIGHT] || kb[SDL_SCANCODE_D]) {
    game_event(&pong->game, EVENT_MOVE_RIGHT);
  }
}

static void disconnect(Pong* pong) {
  LOG_INFO("Disconnecting from current session");
  pong->connection_state.state = DISCONNECTED;
  pong->game_session.state = NOT_IN_LOBBY;

  tcp_shutdown(&pong->tcp_stream);
}

static int prepare_client_message(Pong* pong) {
  ClientMessage msg = {0};

  switch (pong->game_session.state) {
    case NOT_IN_LOBBY:
      msg.id = CREATE_LOBBY;
      strcpy(msg.create_lobby.password, pong->game_session.password);
      LOG_INFO("Sending Create Lobby with pw: %s", msg.create_lobby.password);

      prepare_and_send(pong, &msg);
      pong->game_session.state = WAITING_FOR_LOBBY;
      break;

    case WANT_TO_JOIN: {
      msg.id = JOIN_LOBBY;
      msg.join_lobby.id = pong->game_session.id;
      strcpy(msg.join_lobby.password, pong->game_session.password);
      LOG_INFO("Sending Join Lobby with id: %d and password: %s", msg.join_lobby.id, msg.join_lobby.password);

      prepare_and_send(pong, &msg);
      pong->game_session.state = WAITING_FOR_LOBBY;
      break;

    }
    case WAITING_FOR_LOBBY: {
      break;
    }
    case PLAYING: {
      msg.id = CLIENT_UPDATE;
      msg.client_update.speed = pong->game.player.speed;
      prepare_and_send(pong, &msg);
      break;
    }
    default:
      LOG_FATAL("Unhandled game session state: %d", pong->game_session.state);
  }

  return 0;
}

static int process_server_message(Pong* pong, ServerMessage* message) {
  int res = 0;

  switch (message->id) {
    case LOBBY_CREATED:
      pong->game_session.id = message->lobby_created.id;
      pong->game_session.state = WAITING_FOR_LOBBY;
      LOG_INFO("Game session with id: %d is received", pong->game_session.id);
      break;

    case LOBBY_JOINED:
      strcpy(pong->game_session.opponent_ip, message->lobby_joined.ipv4);
      pong->game_session.state = WAITING_FOR_LOBBY;
      LOG_INFO("Player with IP: %s has joined your session", pong->game_session.opponent_ip);
      break;

    case SERVER_UPDATE:
      if (pong->game_session.state != PLAYING) {
        LOG_INFO("Got first server update, change game_session.state to playing");
        pong->game_session.state = PLAYING;
      }

      pong->game.player.bbox.position.x = message->server_update.player_position.x;
      pong->game.player.bbox.position.y = message->server_update.player_position.y;
      pong->game.opponent.bbox.position.x = message->server_update.opponent_position.x;
      pong->game.opponent.bbox.position.y = message->server_update.opponent_position.y;
      pong->game.ball.bbox.position.x = message->server_update.ball_position.x;
      pong->game.ball.bbox.position.y = message->server_update.ball_position.y;

      break;

    case GAME_STATE_UPDATE:

      if (message->game_state_update.state == STATE_RUNNING) {
        game_event(&pong->game, EVENT_RESTART);
      }

      pong->game.state = message->game_state_update.state;
      break;

    case ERROR_STATUS:
      LOG_ERROR("Error received from server. %d", message->error.status);
      disconnect(pong);
      pong->connection_state.state = LOCAL;
      break;

    default:
      LOG_ERROR("invalid message received from server. Msg id is: %d", message->id);
      return -1;
  }

  return res;
}

static int process_read(Pong* pong) {
  while (true) {
    int n = tcp_recv(&pong->tcp_stream);

    if (n == 0) {
      LOG_WARN("disconnect received");
      return -1;
    }

    if (n < 0) {
      LOG_WARN("recv failed: %s", strerror(errno));
      return -1;
    }

    bool more_to_read = false;
    int offset = 0;
    while (true) {
      ServerMessage message;

      int msg_size = server_message_read(&message, pong->tcp_stream.input + offset,
          pong->tcp_stream.received - offset);

      if (msg_size < 0) {
        LOG_WARN("Invalid message received from server");
        return -1;
      }

      if (msg_size == 0) {
        LOG_INFO("Not enough data to parse message");
        break;
      }

      process_server_message(pong, &message);

      offset += msg_size;

      if (offset == pong->tcp_stream.received) {
        break;
      }
    }

    tcp_consume(&pong->tcp_stream, offset);

    if (pong->tcp_stream.received == sizeof(pong->tcp_stream.input)) {
      more_to_read = true;
    }

    if (!more_to_read) {
      break;
    }
  }

  return 0;
}


static int pong_process_network(Pong* pong, int timeout_ms) {
  unsigned now = SDL_GetTicks();
  unsigned deadline = now + timeout_ms;
  prepare_client_message(pong);

  while (now < deadline) {
    unsigned time_left = deadline - now;
    IOEvent event;
    if (pong->connection_state.state == DISCONNECTED) {
      if (tcp_start_connect(&pong->tcp_stream,
                            pong->connection_state.ip,
                            pong->connection_state.port) == -1) {
        LOG_ERROR("Failed to start connection: %s", strerror(errno));
        return -1;
      }
      pong->connection_state.state = AWAITING_CONNECTION;
    }

    int n = reactor_poll(&pong->reactor, &event, 1, time_left);
    if (n < 0) {
      LOG_ERROR("reactor_poll() failed: %s", strerror(errno));
      return -1;
    }


    if (event.events & IO_EVENT_READ) {
      if (process_read(pong) == -1) {
        LOG_ERROR("process read failed");
        return -1;
      }
    }

    if (event.events & IO_EVENT_WRITE) {
      if (pong->connection_state.state == AWAITING_CONNECTION) {
        int error = tcp_connect(&pong->tcp_stream);
        if (error == 0) {
          LOG_INFO("Successfully connected to %s:%d",
                   pong->connection_state.ip,
                   pong->connection_state.port);
          pong->connection_state.state = CONNECTED;
          tcp_start_recv(&pong->tcp_stream);
        } else {
          LOG_WARN("tcp connect failed");
          pong->connection_state.state = DISCONNECTED;
          return -1;
        }

        pong->connection_state.state = CONNECTED;
      }

      if(tcp_send(&pong->tcp_stream) == -1) {
        LOG_WARN("tcp_send failed: %s", strerror(errno));
        return -1;
      }
    }

    now = SDL_GetTicks();
  }

  return 0;
}

static void pong_render(Pong* pong) {
  Game* game = &pong->game;
  switch (game_state(game)) {
    case STATE_RUNNING: {
      GameObject* objects[] = { &game->player, &game->ball, &game->opponent };
      int n_objects = sizeof(objects) / sizeof(*objects);
      renderer_render(&pong->renderer, objects, game->is_multiplayer ? n_objects : n_objects - 1);
      break;
    }
    case STATE_WON: {
      static GameObject won_screen = {
        .bbox = { .position = { -1.0, -1.0 }, .size = { 2.0, 2.0 } },
        .speed = { 0.0, 0.0 },
        .texture = TEXTURE_WON
      };
      GameObject* objects[] = { &won_screen };
      renderer_render(&pong->renderer, objects, sizeof(objects) / sizeof(*objects));
      break;
    }
    case STATE_LOST: {
      static GameObject lost_screen = {
        .bbox = { .position = { -1.0, -1.0 }, .size = { 2.0, 2.0 } },
        .speed = { 0.0, 0.0},
        .texture = TEXTURE_LOST
      };
      GameObject* objects[] = { &lost_screen };
      renderer_render(&pong->renderer, objects, sizeof(objects) / sizeof(*objects));
      break;
    }
    default:
      LOG_FATAL("UNHANDLED GAME STATE");
      break;
  }
}

static const int TICK_MS = 15;

void pong_run(Pong* pong) {
  pong->running = true;

  while (pong->running) {
    // process events
    game_step_begin(&pong->game);
    pong_process_events(pong);
    // in network case ball position will be updated in pong_process_network
    if (pong->connection_state.state == LOCAL) {
      // TODO: fix multiplayer - update player position
      game_step_end(&pong->game, TICK_MS);
    }

    // render game state
    pong_render(pong);

    // wait for next frame
    if (pong->connection_state.state == LOCAL) {
      // TODO: SDL_Delay to something more precision (usleep)
      // Note: Count on a delay granularity of at least 10 ms. Some platforms have shorter clock ticks but this is the most common.
      SDL_Delay(TICK_MS);
    }
    else {
      pong_process_network(pong, TICK_MS);
    }
  }
}
