#include "pong.h"
#include "log.h"

#include <SDL2/SDL_video.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_timer.h>


int pong_init(Pong* pong, const char* ip, unsigned short port) {
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

  pong->connection_state.state = LOCAL;
  strcpy(pong->connection_state.ip, ip);
  pong->connection_state.port = port;

  network_session_init(&pong->network_session);

  pong->running = true;
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

void pong_run(Pong* pong) {
  while (pong->running) {
    // process events
    game_step_begin(&pong->game);
    pong_process_events(pong);
    game_step_end(&pong->game, 16 /* ms */);

    // render game state
    renderer_render(&pong->renderer, &pong->game);

    // wait for next frame
    // TODO: proper game loop time management
    SDL_Delay(16);
  }
}
