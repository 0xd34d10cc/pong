#include <stdlib.h>

#ifdef _WIN32
#define SDL_MAIN_HANDLED
#endif
#include <SDL2/SDL.h>

#include "game.h"
#include "renderer.h"
#include "log.h"


int main(int argc, char* argv[]) {
  if (argc > 1) {
    LOG_ERROR("Unexpected number of arguments: got %d, expected 0", argc - 1);
    return EXIT_FAILURE;
  }

#ifdef _WIN32
  SDL_SetMainReady();
#endif

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    LOG_ERROR("Could not initialize SDL: %s", SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_version v;
  SDL_GetVersion(&v);
  LOG_INFO("%s SDL %d.%d.%d", SDL_GetPlatform(), v.major, v.minor, v.patch);

  // init context (window, renderer, game state)
  SDL_Window* window = SDL_CreateWindow(
      "pong",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      DEFAULT_WINDOW_WIDTH,
      DEFAULT_WINDOW_HEIGHT,
      0 // todo: flags
  );

  if (!window) {
    LOG_ERROR("Failed to create window: %s", SDL_GetError());
    return EXIT_FAILURE;
  }

  Renderer renderer;
  if (renderer_init(&renderer, window)) {
    return EXIT_FAILURE;
  }

  Game game;
  game_init(&game, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);

  bool running = true;
  while (running) {
    game_step_begin(&game);

    // process events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          break;
        case SDL_KEYDOWN:
          switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_Q:
              if (SDL_GetModState() & KMOD_LCTRL) {
                running = false;
              }
              break;
            case SDL_SCANCODE_RETURN:
              game_event(&game, EVENT_RESTART);
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }
    }

    const Uint8* kb = SDL_GetKeyboardState(NULL);
    if (kb[SDL_SCANCODE_LEFT] || kb[SDL_SCANCODE_A]) {
      game_event(&game, EVENT_MOVE_LEFT);
    }
    else if (kb[SDL_SCANCODE_RIGHT] || kb[SDL_SCANCODE_D]) {
      game_event(&game, EVENT_MOVE_RIGHT);
    }

    // update state
    game_step_end(&game, 16);

    // render
    renderer_render(&renderer, &game);

    // wait for next frame
    // todo: proper game loop time management
    SDL_Delay(16);
  }

  renderer_close(&renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  LOG_INFO("Closed successfully");
  return EXIT_SUCCESS;
}
