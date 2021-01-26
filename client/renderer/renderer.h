#ifndef RENDERER_H
#define RENDERER_H

#include "types.h"
#include "shader.h"
#include "buffer.h"
#include "texture.h"

#include "game/object.h"
#include "game/texture_id.h"

typedef struct SDL_Window SDL_Window;
typedef void* SDL_GLContext;

typedef struct {
  // Vertex attributes
  AttributeID pos;   // position of the vertex
  AttributeID uv;    // texture coordinates
  AttributeID color; // color of the vertex

  // Uniform variables
  AttributeID projection; // projection matrix
  AttributeID texture;    // texture id
} Attributes;

typedef struct {
  SDL_Window* window;
  SDL_GLContext* context;

  Shader     shader;
  Attributes attributes; // shader attributes
  Texture    textures[TEXTURE_MAX];

  ObjectID vertex_array; // array of buffer objects
  VertexBuffer vertices; // actual vertex data (positions, texture mappings, colors)
  IndexBuffer indices;   // information about order of rendering of vertices
} Renderer;

// returns 0 on success
int renderer_init(Renderer* renderer, SDL_Window* window);
void renderer_render(Renderer* renderer, GameObject** objects, int n);
void renderer_close(Renderer* renderer);

#endif // RENDERER_H
