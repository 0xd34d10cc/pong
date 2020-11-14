#include "renderer.h"
#include "game.h"

#include <SDL2/SDL_render.h>


const char* renderer_init(Renderer* renderer, SDL_Window* window) {
  renderer->backend = SDL_CreateRenderer(window, -1 /* any gpu */, 0 /* todo: flags */);
  if (!renderer->backend) {
    return SDL_GetError();
  }

  renderer->lost_image = SDL_LoadBMP("lose.bmp");
  if (!renderer->lost_image) {
    return SDL_GetError();
  }

  renderer->lost_texture = SDL_CreateTextureFromSurface(renderer->backend, renderer->lost_image);
  if (!renderer->lost_texture) {
    return SDL_GetError();
  }

  return NULL;
}

void renderer_close(Renderer* renderer) {
  SDL_DestroyTexture(renderer->lost_texture);
  SDL_FreeSurface(renderer->lost_image);
  SDL_DestroyRenderer(renderer->backend);
}

static void render_lost(Renderer* renderer) {
  SDL_RenderCopy(renderer->backend, renderer->lost_texture, NULL, NULL);
}

static void render_running(Renderer* renderer, Game* game) {
  int ball_x;
  int ball_y;
  int player_mid;
  game_positions(game, &player_mid, &ball_x, &ball_y);

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

  SDL_SetRenderDrawColor(renderer->backend, 0xb0, 0xb0, 0xb0, 0xff);
  SDL_RenderFillRect(renderer->backend, &ball);
  SDL_RenderFillRect(renderer->backend, &player);
}

void renderer_render(Renderer* renderer, Game* game) {
  SDL_SetRenderDrawColor(renderer->backend, 0, 0, 0, 0xff);
  SDL_RenderClear(renderer->backend);

  switch (game_state(game)) {
    case STATE_LOST:
      render_lost(renderer);
      break;
    case STATE_RUNNING:
      render_running(renderer, game);
      break;
  }

  SDL_RenderPresent(renderer->backend);
}