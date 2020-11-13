#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef _WIN32
#define SDL_MAIN_HANDLED
#endif
#include <SDL2/SDL.h>

typedef int bool;
#define true 1
#define false 0

static const int DEFAULT_WINDOW_WIDTH = 800;
static const int DEFAULT_WINDOW_HEIGHT = 600;

static const int PLAYER_WIDTH = 100;
static const int PLAYER_HEIGHT = 15;
static const int PLAYER_SPEED = 5;

static int clamp(int x, int min, int max) {
  if (x < min) return min;
  if (x > max) return max;
  return x;
}

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

  SDL_Rect player = {
    .x = DEFAULT_WINDOW_WIDTH / 2 - PLAYER_WIDTH / 2,
    .y = DEFAULT_WINDOW_HEIGHT - PLAYER_HEIGHT,
    .w = PLAYER_WIDTH,
    .h = PLAYER_HEIGHT
  };

  bool running = true;
  while (running) {
    int movement = 0;

    // process events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          break;
        case SDL_KEYDOWN:
          switch (event.key.keysym.sym) {
            case SDLK_LEFT:
            case SDLK_a:
              movement = -PLAYER_SPEED;
              break;
            case SDLK_RIGHT:
            case SDLK_d:
              movement = PLAYER_SPEED;
              break;
          }
          break;
      }
    }

    int window_height;
    int window_width;
    SDL_GetWindowSize(window, &window_width, &window_height);

    // update state
    player.x = clamp(player.x + movement, 0, window_width - PLAYER_WIDTH);

    // render
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0xb0, 0xb0, 0xb0, 0xff);
    SDL_RenderFillRect(renderer, &player);
    SDL_RenderPresent(renderer);

    // wait for next frame
    SDL_Delay(16);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  game_log("Closed successfully");
  return EXIT_SUCCESS;
}
