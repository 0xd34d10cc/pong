#ifndef RENDERER_H
#define RENDERER_H

#include "shader.h"
#include "vgl.h"

typedef struct Game Game;
typedef struct SDL_Window SDL_Window;
typedef void* SDL_GLContext;

// OpenGL object identifier
typedef unsigned int ObjectID;
// Shader attribute identifer
typedef unsigned int AttributeID;

typedef struct {
  // Vertex attributes
  AttributeID pos;   // position of the vertex
  AttributeID uv;    // texture mappings
  AttributeID color; // color of the vertex

  // Uniform variables
  AttributeID projection; // projection matrix
  AttributeID texture;    // texture id
} Attributes;

typedef struct {
  SDL_GLContext* context;
  VGL vgl;

  Shader     shader;
  Attributes attributes; // shader attributes

  ObjectID vertex_array;  // array of vertices buffers
  ObjectID vertices;      // actual vertex data (positions, texture mappings, colors)
  ObjectID indices;       // information about order of rendering of vertices
} Renderer;

// returns 0 on success
int renderer_init(Renderer* renderer, SDL_Window* window);
void renderer_render(Renderer* renderer, Game* game);
void renderer_close(Renderer* renderer);

#endif // RENDERER_H
