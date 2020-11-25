#include "pong.h"
#include "log.h"
#include "game/protocol.h"
#include "bool.h"

#include <SDL2/SDL_video.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_timer.h>


#define RECONNECT_DELAY 1000 * 3

int pong_init(Pong* pong, LaunchParams* params) {
  pong->running = false;
  pong->window = SDL_CreateWindow(
      "pong",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      DEFAULT_WINDOW_WIDTH,
      DEFAULT_WINDOW_HEIGHT,
      SDL_WINDOW_SHOWN
  );

  if (!pong->window) {
    LOG_ERROR("Failed to create window: %s", SDL_GetError());
    return -1;
  }

  if (renderer_init(&pong->renderer, pong->window)) {
    return -1;
  }

  game_init(&pong->game, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);

  if (reactor_init(&pong->reactor)) {
    LOG_ERROR("Can't initialize reactor");
    return -1;
  }

  pong->connection_state.state = DISCONNECTED;
  strcpy(pong->connection_state.ip, params->ip);
  pong->connection_state.port = params->port;

  tcp_init(&pong->tcp_stream, &pong->reactor);

  pong->game_session.id = params->session_id;
  strcpy(pong->game_session.password, params->password);
  // TODO: pass game session state instead of game mode
  switch (params->game_mode) {
    case LOCAL_GAME:
      // TODO: We have two LOCAL in different enum, could be fuckup
      pong->game_session.state = LOCAL;
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
  return 0;
}

void pong_close(Pong* pong) {
  renderer_close(&pong->renderer);
  SDL_DestroyWindow(pong->window);
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
          game_event(&pong->game, EVENT_RESTART);
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

static int process_server_message(Pong* pong, ServerMessage* message, int timeout) {
  int res = 0;

  switch (message->id) {
    case LOBBY_CREATED:
      pong->game_session.id = message->lobby_created.id;
      pong->game_session.state = CREATED;
      LOG_INFO("Game session with id: %d is received", pong->game_session.id);
      break;

    case LOBBY_JOINED:
      strcpy(pong->game_session.opponent_ip, message->lobby_joined.ipv4);
      pong->game_session.state = JOINED;
      LOG_INFO("Player with IP: %s has joined your session", pong->game_session.opponent_ip);
      break;
  }

  return res;
}

static int process_read(Pong* pong, int timeout) {
  while (true) {
    int n = tcp_recv(&pong->tcp_stream);

    if (n == 0) {
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

      process_server_message(pong, &message, timeout);

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

static int process_lobby(Pong* pong, int timeout) {
  ClientMessage msg = {0};
  char buf[MAX_MESSAGE_SIZE];

  switch (pong->game_session.state) {
    case NOT_IN_LOBBY:
      msg.id = CREATE_LOBBY;
      strcpy(msg.create_lobby.password, pong->game_session.password);

      int n = client_message_write(&msg, buf, sizeof(buf));

      if (n == 0) {
        return -1;
      }

      int send_res = tcp_start_send(&pong->tcp_stream, buf, n);

      if (send_res <= 0) {
        LOG_WARN("Send operation failed with code: %d, msg: %s", send_res, strerror(errno));
        return -1;
      }

      pong->game_session.state = WAITING_FOR_LOBBY;
      break;

    case WANT_TO_JOIN: {
      msg.id = JOIN_LOBBY;
      msg.join_lobby.id = pong->game_session.id;
      strcpy(msg.join_lobby.password, pong->game_session.password);
      int n = client_message_write(&msg, buf, sizeof(buf));

      if (n == 0) {
        return -1;
      }

      int send_res = tcp_start_send(&pong->tcp_stream, buf, n);

      if (send_res <= 0) {
        LOG_WARN("Send operation failed with code: %d, msg %s", send_res, strerror(errno));
        return -1;
      }

      pong->game_session.state = WAITING_FOR_LOBBY;
      break;

    }
    case WAITING_FOR_LOBBY: {

      IOEvent event;
      int n_events = reactor_poll(&pong->reactor, &event, 1, timeout);
      if (n_events == -1) {
        LOG_ERROR("reactor_poll failed: %s", strerror(errno));
        return -1;
      }

      if (n_events == 0) {
        return 0;
      }

      if (event.events & IO_EVENT_READ) {
        process_read(pong, timeout);
      }

      if (event.events & IO_EVENT_WRITE) {
        if(tcp_send(&pong->tcp_stream) == -1) {
          LOG_WARN("tcp_send failed: %s", strerror(errno));
          return -1;
        }
      }

      break;
    }

  }

  return 0;
}

static int pong_process_network(Pong* pong, int timeout_ms) {
  unsigned now = SDL_GetTicks();
  unsigned deadline = now + timeout_ms;

  while (now < deadline) {
    unsigned time_left = deadline - now;
    switch (pong->connection_state.state) {
      case DISCONNECTED:
        if (tcp_start_connect(&pong->tcp_stream,
                              pong->connection_state.ip,
                              pong->connection_state.port) == -1) {
          LOG_ERROR("Failed to start connection: %s", strerror(errno));
          return -1;
        }
        reactor_update(&pong->reactor, &pong->tcp_stream.state, IO_EVENT_WRITE);
        pong->connection_state.state = AWAITING_CONNECTION;
        break;
      case AWAITING_CONNECTION: {
        IOEvent event;
        int n = reactor_poll(&pong->reactor, &event, 1, time_left);
        if (n < 0) {
          LOG_ERROR("reactor_poll() failed: %s", strerror(errno));
          return -1;
        }

        if (n == 1 && event.events & IO_EVENT_WRITE) {
          reactor_update(&pong->reactor, &pong->tcp_stream.state, 0);
          int error = tcp_connect(&pong->tcp_stream);
          if (error == 0) {
            LOG_INFO("Successfully connected to %s:%d",
                     pong->connection_state.ip,
                     pong->connection_state.port);
            pong->connection_state.state = CONNECTED;
            tcp_start_recv(&pong->tcp_stream);

          }
          else {
            LOG_ERROR("Failed to connect to %s:%d: %s",
                       pong->connection_state.ip,
                       pong->connection_state.port,
                       strerror(error));
            pong->connection_state.state = DISCONNECTED;

            // TODO: Should be only in case of NONLOCAL game
            SDL_Delay(RECONNECT_DELAY);
          }
        }
        break;
      }
      case CONNECTED:

        process_lobby(pong, time_left);
        SDL_Delay(time_left);
        break;
    }

    now = SDL_GetTicks();
  }
  return 0;
}

static const int TICK_MS = 16;

void pong_run(Pong* pong) {
  pong->running = true;

  while (pong->running) {
    // process events
    game_step_begin(&pong->game);
    pong_process_events(pong);
    game_step_end(&pong->game, TICK_MS);

    // render game state
    renderer_render(&pong->renderer, &pong->game);

    // wait for next frame
    if (pong->connection_state.state == LOCAL) {
      SDL_Delay(TICK_MS);
    }
    else {
      pong_process_network(pong, TICK_MS);
    }
  }
}
