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

static const int BALL_WIDTH = 30;
static const int BALL_HEIGHT = 30;


static int clamp(int x, int min, int max) {
  if (x < min) return min;
  if (x > max) return max;
  return x;
}

static bool in_range(int x, int left, int right) {
  return !(x < left || x > right);
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

  SDL_Rect ball = {
    .x = DEFAULT_WINDOW_WIDTH / 2,
    .y = DEFAULT_WINDOW_HEIGHT / 2,
    .w = BALL_WIDTH,
    .h = BALL_HEIGHT
  };
  
  // TODO: add path checking
  SDL_Surface* image = SDL_LoadBMP("lose.bmp");
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, image);

  int ball_speed_x = PLAYER_SPEED - 2;
  int ball_speed_y = PLAYER_SPEED - 2;
  bool lost = false;
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
          switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_Q:
              if (SDL_GetModState() & KMOD_LCTRL) {
                running = false;
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

    if (lost) {
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
	SDL_Delay(100);
	continue;
    }
    const Uint8* state = SDL_GetKeyboardState(NULL);
    if (state[SDL_SCANCODE_LEFT] || state[SDL_SCANCODE_A]) {
      movement = -PLAYER_SPEED;
    }

    if (state[SDL_SCANCODE_RIGHT] || state[SDL_SCANCODE_D]) {
      movement = PLAYER_SPEED;
    }

    // update state
    player.x = clamp(player.x + movement, 0, DEFAULT_WINDOW_WIDTH - PLAYER_WIDTH);
    ball.x = clamp(ball.x + ball_speed_x, 0, DEFAULT_WINDOW_WIDTH - BALL_WIDTH); 
    ball.y = clamp(ball.y + ball_speed_y, 0, DEFAULT_WINDOW_HEIGHT - BALL_HEIGHT);

    // floor/wall hit handling
    if (ball.x == 0 || ball.x == DEFAULT_WINDOW_WIDTH - BALL_WIDTH) {
	ball_speed_x *= -1;
	game_log("X hit");
    }
    
    if (ball.y == 0 || ball.y == DEFAULT_WINDOW_HEIGHT - BALL_HEIGHT) {
    	ball_speed_y *= -1;
	game_log("Y hit");
	lost = ball.y == DEFAULT_WINDOW_HEIGHT - BALL_HEIGHT;
    }

    int low_bound_y = ball.y + BALL_HEIGHT;
    // player hit
    if (in_range(low_bound_y, player.y, player.y+ball_speed_y) && 
	in_range(ball.x, player.x, player.x+PLAYER_WIDTH)) {
	game_log("y's are same");
	ball_speed_y *= -1;
	ball_speed_y < 0 ? ball_speed_y-- : ball_speed_y++;
	ball_speed_x++;
	// not sure if it's needed
	//ball_speed_x *= -1;
    }
    // render
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
