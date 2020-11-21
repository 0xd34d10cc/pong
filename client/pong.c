#include "pong.h"
#include "log.h"
#include "game/protocol.h"

#include <SDL2/SDL_video.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_timer.h>


int pong_init(Pong* pong, const char* ip, unsigned short port) {
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
  strcpy(pong->connection_state.ip, ip);
  pong->connection_state.port = port;

  tcp_init(&pong->tcp_stream, &pong->reactor);

  pong->game_session.id = -1;
  pong->game_session.state = NOT_IN_LOBBY;
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

static int pong_process_network(Pong* pong, int timeout_ms) {
  unsigned now = SDL_GetTicks();
  unsigned deadline = now + timeout_ms;

  while (now < deadline) {
    unsigned time_left = deadline - now;
    switch (pong->connection_state.state) {
      case DISCONNECTED:
        if (tcp_start_connect(&pong->tcp_stream, pong->connection_state.ip, pong->connection_state.port) == -1) {
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
            LOG_INFO("Successfully connected to %s:%d", pong->connection_state.ip, pong->connection_state.port);
            pong->connection_state.state = CONNECTED;
            tcp_start_recv(&pong->tcp_stream);
          }
          else {
            LOG_ERROR("Failed to connect to %s:%d: %s",
                       pong->connection_state.ip,
                       pong->connection_state.port,
                       strerror(error));
            pong->connection_state.state = DISCONNECTED;
            // TODO: add delay before another attempt to reconnect
          }
        }
        break;
      }
      case CONNECTED:
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
