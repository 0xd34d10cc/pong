#ifndef VGL_H
#define VGL_H

#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>


// vtable of OpenGL functions
typedef struct VGL {
  // Create a new shader instance
  PFNGLCREATESHADERPROC glCreateShader;
  // Delete shader instance
  PFNGLDELETESHADERPROC glDeleteShader;
  // Set source code for shader
  PFNGLSHADERSOURCEPROC glShaderSource;
  // Compile the source code
  PFNGLCOMPILESHADERPROC glCompileShader;
  // Get shader attributes (e.g. compilation status)
  PFNGLGETSHADERIVPROC glGetShaderiv;
  // Get shader compilation error
  PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;

  // Create a new program instance
  // Program = vertex shader + fragment shader linked together
  PFNGLCREATEPROGRAMPROC glCreateProgram;
  // Delete program instance
  PFNGLDELETEPROGRAMPROC glDeleteProgram;
  // Attach shader to a program
  PFNGLATTACHSHADERPROC glAttachShader;
  // Detach shader from a program
  PFNGLDETACHSHADERPROC glDetachShader;
  // Link shaders in a program
  PFNGLLINKPROGRAMPROC glLinkProgram;
  // Get program attributes (e.g. link status)
  PFNGLGETPROGRAMIVPROC glGetProgramiv;

  // Get ID of uniform (shared across compute units) variable
  PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
  // Get ID of attribute (unique to each compute unit) variable
  PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;

  // Allocate new vertex arrays (array of vertex buffers, aka vao)
  PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
  // Delete vertex arrays
  PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
  // Set vertex array as "current"
  PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
  // Allocate new vertex buffers (aka vbo)
  PFNGLGENBUFFERSPROC glGenBuffers;
  // Delete vertex buffers
  PFNGLDELETEBUFFERSPROC glDeleteBuffers;
  // Set vertex buffer as "current"
  PFNGLBINDBUFFERPROC glBindBuffer;
  // Transfer data to buffer
  PFNGLBUFFERDATAPROC glBufferData;
  // Map buffer data in virtual memory (write only)
  PFNGLMAPBUFFERPROC glMapBuffer;
  // Unmap buffer data from virtual memory
  PFNGLUNMAPBUFFERPROC glUnmapBuffer;
  // Bind shader attribute to "current" vertex array
  PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
  // Set offset for shader attribute at which the value is located
  PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
} VGL;

int vgl_load(void);
int vgl_init(VGL* vgl);

extern VGL vgl;

#endif // VGL_H
