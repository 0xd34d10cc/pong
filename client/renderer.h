#ifndef RENDERER_H
#define RENDERER_H

typedef struct Game Game;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Surface SDL_Surface;
typedef struct SDL_Texture SDL_Texture;

typedef struct {
    SDL_Renderer* backend;
    SDL_Surface* lost_image;
    SDL_Texture* lost_texture;

    SDL_Surface* won_image;
    SDL_Texture* won_texture;
} Renderer;

// returns 0 on success
int renderer_init(Renderer* renderer, SDL_Window* window);
void renderer_render(Renderer* renderer, Game* game);
void renderer_close(Renderer* renderer);

#endif // RENDERER_H