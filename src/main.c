#include <stdio.h>
#include <stdarg.h>

#include <SDL2/SDL.h>


typedef int bool;
#define true 1
#define false 0

#ifdef __linux__
#include <unistd.h>
#endif

static void sleep_ms(int ms) {
  // todo: implementation for windows
  useconds_t usec = ms * 1000;
  usleep(usec);
}

// todo: add levels
// todo: log time
static void pong_log(const char* format, ...) {
  char buffer[1024];

  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  fprintf(stderr, "[log] %s\n", buffer);
}

int main() {
  // init SDL2
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    pong_log("Could not initialize SDL: %s", SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_version v;
  SDL_GetVersion(&v);
  pong_log("SDL version is %d.%d.%d", v.major, v.minor, v.patch);

  // init context (window, renderer, game state)
  SDL_Window* window = SDL_CreateWindow(
      "pong",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      800, 600, 0 // todo: flags
  );

  if (!window) {
    pong_log("Failed to create window: %s", SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1 /* todo: gpu index */, 0 /* todo: flags */);
  if (!renderer) {
    pong_log("Failed to create renderer: %s", SDL_GetError());
    return EXIT_FAILURE;
  }

  bool running = true;
  while (running) {
    // process events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          break;
      }
    }

    int window_height;
    int window_width;
    SDL_GetWindowSize(window, &window_width, &window_height);

    // update state
    static const int PLAYER_WIDTH = 100;
    static const int PLAYER_HEIGHT = 20;

    SDL_Rect player = {
      .x = 300,
      .y = window_height - PLAYER_HEIGHT,
      .w = PLAYER_WIDTH,
      .h = PLAYER_HEIGHT,
    };

    // render
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0xb0, 0xb0, 0xb0, 0xff);
    SDL_RenderFillRect(renderer, &player);
    SDL_RenderPresent(renderer);

    // wait for next frame
    sleep_ms(16);
  }

  SDL_DestroyWindow(window);
  SDL_DestroyRenderer(renderer);
  SDL_Quit();
  pong_log("Closed successfully");
  return EXIT_SUCCESS;
}
