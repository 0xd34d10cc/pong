#include "renderer.h"
#include "game/game.h"
#include "log.h"

#include <SDL2/SDL_video.h>


int renderer_init(Renderer* renderer, SDL_Window* window) {
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

#ifdef PONG_DEBUG
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

  // Enable VSync, not sure if it actually should be enabled by default
  SDL_GL_SetSwapInterval(1);

  SDL_GLContext* context = SDL_GL_CreateContext(window);
  if (!context) {
    LOG_ERROR("Failed to initialie OpenGL context: %s", SDL_GetError());
    return -1;
  }


  renderer->context = context;
  return 0;
}

void renderer_close(Renderer* renderer) {
  SDL_GL_DeleteContext(renderer->context);
}

static void render_lost(Renderer* renderer) {
  (void)renderer;
  // draw lose screen
}

static void render_running(Renderer* renderer, Game* game) {
  (void)renderer;
  (void)game;
  // draw player, ball and opponent
}

void renderer_render(Renderer* renderer, Game* game) {
  // clear screen

  switch (game_state(game)) {
    case STATE_LOST:
      render_lost(renderer);
      break;
    case STATE_RUNNING:
      render_running(renderer, game);
      break;
    case STATE_WON:
      // TODO: render
      break;
  }

  // swap
}
