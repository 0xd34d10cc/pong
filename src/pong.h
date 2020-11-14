#ifndef PONG_H
#define PONG_H

#include "renderer.h"
#include "game.h"

typedef struct SDL_Window SDL_Window;

typedef struct Pong {
  bool running;
  SDL_Window* window;
  Renderer renderer;
  Game game;
} Pong;

// returns 0 on success
int pong_init(Pong* pong);
void pong_run(Pong* pong);
void pong_close(Pong* pong);

#endif // PONG_H