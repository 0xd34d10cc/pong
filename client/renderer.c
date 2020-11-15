#include "renderer.h"
#include "game.h"
#include "log.h"

#include <SDL2/SDL_render.h>


int renderer_init(Renderer* renderer, SDL_Window* window) {
  renderer->backend = SDL_CreateRenderer(window, -1 /* any gpu */, 0 /* todo: flags */);
  if (!renderer->backend) {
    LOG_ERROR("Failed to create renderer: %s", SDL_GetError());
    return -1;
  }

  // todo: embed the image into binary
  renderer->lost_image = SDL_LoadBMP("resources/lose.bmp");
  if (!renderer->lost_image) {
    LOG_ERROR("Failed to load loser image: %s", SDL_GetError());
    return -1;
  }

  renderer->lost_texture = SDL_CreateTextureFromSurface(renderer->backend, renderer->lost_image);
  if (!renderer->lost_texture) {
    LOG_ERROR("Failed to create texture: %s", SDL_GetError());
    return -1;
  }

  return 0;
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
  SDL_Rect player = {
    .x = 0,
    .y = DEFAULT_WINDOW_HEIGHT - PLAYER_HEIGHT,
    .w = PLAYER_WIDTH,
    .h = PLAYER_HEIGHT
  };

  SDL_Rect ball = {
    .x = 0,
    .y = 0,
    .w = BALL_WIDTH,
    .h = BALL_HEIGHT
  };

  game_positions(game, &player.x, &ball.x, &ball.y);

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
