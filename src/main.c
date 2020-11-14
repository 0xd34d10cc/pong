#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef _WIN32
#define SDL_MAIN_HANDLED
#endif
#include <SDL2/SDL.h>

#include "game.h"

static const int DEFAULT_WINDOW_WIDTH = 800;
static const int DEFAULT_WINDOW_HEIGHT = 600;

// todo: add levels
// todo: log time
static void game_log(const char* format, ...) {
  char buffer[1024];

  va_list args;
  va_start(args, format);
  int n = vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  const char* fmt = n <= sizeof(buffer) ? "[log] %s\n" : "[log] %s...\n";
  fprintf(stderr, fmt, buffer);
}

int main(int argc, char* argv[]) {
  if (argc > 1) {
    game_log("Unexpected number of arguments: got %d, expected 0", argc - 1);
    return EXIT_FAILURE;
  }

#ifdef _WIN32
  SDL_SetMainReady();
#endif

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    game_log("Could not initialize SDL: %s", SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_version v;
  SDL_GetVersion(&v);
  game_log("%s SDL %d.%d.%d", SDL_GetPlatform(), v.major, v.minor, v.patch);

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
    game_log("Failed to create window: %s", SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1 /* todo: gpu index */, 0 /* todo: flags */);
  if (!renderer) {
    game_log("Failed to create renderer: %s", SDL_GetError());
    return EXIT_FAILURE;
  }

  // TODO: add path checking
  SDL_Surface* image = SDL_LoadBMP("lose.bmp");
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, image);

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

    GameState state = game_state(&game);
    if (state == LOST) {
      SDL_RenderCopy(renderer, texture, NULL, NULL);
      SDL_RenderPresent(renderer);
      SDL_Delay(100);
      continue;
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
    int ball_x;
    int ball_y;
    int player_mid;
    game_positions(&game, &player_mid, &ball_x, &ball_y);

    SDL_Rect player = {
      .x = player_mid - PLAYER_WIDTH / 2,
      .y = DEFAULT_WINDOW_HEIGHT - PLAYER_HEIGHT,
      .w = PLAYER_WIDTH,
      .h = PLAYER_HEIGHT
    };

    SDL_Rect ball = {
      .x = ball_x,
      .y = ball_y,
      .w = BALL_WIDTH,
      .h = BALL_HEIGHT
    };

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0xb0, 0xb0, 0xb0, 0xff);
    SDL_RenderFillRect(renderer, &ball);
    SDL_RenderFillRect(renderer, &player);
    SDL_RenderPresent(renderer);

    // wait for next frame
    // todo: proper game loop time management
    SDL_Delay(16);
  }

  SDL_DestroyTexture(texture);
  SDL_FreeSurface(image);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  game_log("Closed successfully");
  return EXIT_SUCCESS;
}
