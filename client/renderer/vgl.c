#include "vgl.h"

#include <SDL2/SDL_video.h>

#define TRY_LOAD(name)                         \
  do {                                         \
    void* ptr = SDL_GL_GetProcAddress(#name);  \
    if (!ptr) {                                \
      return -1;                               \
    }                                          \
    table->name = ptr;                         \
  } while (0)

int vgl_init(VGL* table) {
  TRY_LOAD(glCreateShader);
  TRY_LOAD(glDeleteShader);
  TRY_LOAD(glShaderSource);
  TRY_LOAD(glCompileShader);
  TRY_LOAD(glGetShaderiv);
  TRY_LOAD(glGetShaderInfoLog);

  TRY_LOAD(glCreateProgram);
  TRY_LOAD(glDeleteProgram);
  TRY_LOAD(glAttachShader);
  TRY_LOAD(glDetachShader);
  TRY_LOAD(glLinkProgram);
  TRY_LOAD(glGetProgramiv);
  TRY_LOAD(glUseProgram);

  TRY_LOAD(glGetUniformLocation);
  TRY_LOAD(glGetAttribLocation);
  TRY_LOAD(glUniformMatrix4fv);

  TRY_LOAD(glGenVertexArrays);
  TRY_LOAD(glDeleteVertexArrays);
  TRY_LOAD(glBindVertexArray);
  TRY_LOAD(glGenBuffers);
  TRY_LOAD(glDeleteBuffers);
  TRY_LOAD(glBindBuffer);
  TRY_LOAD(glBufferData);
  TRY_LOAD(glMapBuffer);
  TRY_LOAD(glUnmapBuffer);
  TRY_LOAD(glEnableVertexAttribArray);
  TRY_LOAD(glVertexAttribPointer);

  return 0;
}

VGL vgl = { 0 };

int vgl_load(void) {
  return vgl_init(&vgl);
}
