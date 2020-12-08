#ifndef RENDERER_H
#define RENDERER_H

typedef struct Game Game;
typedef struct SDL_Window SDL_Window;
typedef void* SDL_GLContext;

typedef struct {
  SDL_GLContext* context;
} Renderer;

// returns 0 on success
int renderer_init(Renderer* renderer, SDL_Window* window);
void renderer_render(Renderer* renderer, Game* game);
void renderer_close(Renderer* renderer);

#endif // RENDERER_H
