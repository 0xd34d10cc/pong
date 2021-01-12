#ifndef RENDERER_H
#define RENDERER_H

#include "types.h"
#include "shader.h"
#include "buffer.h"
#include "texture.h"

typedef struct Game Game;
typedef struct SDL_Window SDL_Window;
typedef void* SDL_GLContext;

typedef struct {
  // Vertex attributes
  AttributeID pos;   // position of the vertex
  AttributeID color; // color of the vertex

  // Uniform variables
  AttributeID projection; // projection matrix
} Attributes;

typedef struct {
  SDL_Window* window;
  SDL_GLContext* context;

  Shader     shader;
  Attributes attributes; // shader attributes

  ObjectID vertex_array; // array of buffer objects
  VertexBuffer vertices; // actual vertex data (positions, texture mappings, colors)
  IndexBuffer indices;   // information about order of rendering of vertices
} Renderer;

// returns 0 on success
int renderer_init(Renderer* renderer, SDL_Window* window);
void renderer_render(Renderer* renderer, Game* game);
void renderer_close(Renderer* renderer);

#endif // RENDERER_H
