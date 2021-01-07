#include "shader.h"

#include "log.h"
#include "vgl.h"


static const ProgramID INVALID_SHADER = (ProgramID)-1;

static ProgramID compile(const char* source, int type) {
  ProgramID shader = vgl.glCreateShader(type);
  vgl.glShaderSource(shader, 1, &source, NULL);
  vgl.glCompileShader(shader);

  GLint status = GL_FALSE;
  vgl.glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    char message[4096];
    vgl.glGetShaderInfoLog(shader, sizeof(message), NULL, message);
    LOG_ERROR("Failed to compile shader: %s", message);
    return INVALID_SHADER;
  }

  return shader;
}

// FIXME: leaks shader/program in case of error
int shader_compile(Shader* shader, const char* vertex_source, const char* fragment_source) {
  ProgramID vertex = compile(vertex_source, GL_VERTEX_SHADER);
  if (vertex == INVALID_SHADER) {
    return -1;
  }

  ProgramID fragment = compile(fragment_source, GL_FRAGMENT_SHADER);
  if (fragment == INVALID_SHADER) {
    return -1;
  }

  ProgramID program = vgl.glCreateProgram();
  vgl.glAttachShader(program, vertex);
  vgl.glAttachShader(program, fragment);
  vgl.glLinkProgram(program);

  GLint status = GL_FALSE;
  vgl.glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    LOG_ERROR("Failed to link shaders together");
    return -1;
  }

  shader->vertex = vertex;
  shader->fragment = fragment;
  shader->program = program;
  return 0;
}

AttributeID shader_uniform(Shader* shader, const char* name) {
  return vgl.glGetUniformLocation(shader->program, name);
}

AttributeID shader_var(Shader* shader, const char* name) {
  return vgl.glGetAttribLocation(shader->program, name);
}

void shader_release(Shader* shader) {
  vgl.glDetachShader(shader->program, shader->vertex);
  vgl.glDetachShader(shader->program, shader->fragment);
  vgl.glDeleteProgram(shader->program);
  vgl.glDeleteShader(shader->vertex);
  vgl.glDeleteShader(shader->fragment);
}
