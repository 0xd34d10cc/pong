#include "shader.h"

#include "log.h"
#include "vgl.h"


static const ProgramID INVALID_SHADER = (ProgramID)-1;

static ProgramID compile(VGL* table, const char* source, int type) {
  ProgramID shader = table->glCreateShader(type);
  table->glShaderSource(shader, 1, &source, NULL);
  table->glCompileShader(shader);

  GLint status = GL_FALSE;
  table->glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    char message[4096];
    table->glGetShaderInfoLog(shader, sizeof(message), NULL, message);
    LOG_ERROR("Failed to compile shader: %s", message);
    return INVALID_SHADER;
  }

  return shader;
}

// FIXME: leaks shader/program in case of error
int shader_compile(Shader* shader, VGL* table, const char* vertex_source, const char* fragment_source) {
  ProgramID vertex = compile(table, vertex_source, GL_VERTEX_SHADER);
  if (vertex == INVALID_SHADER) {
    return -1;
  }

  ProgramID fragment = compile(table, fragment_source, GL_FRAGMENT_SHADER);
  if (fragment == INVALID_SHADER) {
    return -1;
  }

  ProgramID program = table->glCreateProgram();
  table->glAttachShader(program, vertex);
  table->glAttachShader(program, fragment);
  table->glLinkProgram(program);

  GLint status = GL_FALSE;
  table->glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    LOG_ERROR("Failed to link shaders together");
    return -1;
  }

  shader->vertex = vertex;
  shader->fragment = fragment;
  shader->program = program;
  return 0;
}

void shader_release(Shader* shader, VGL* table) {
  table->glDetachShader(shader->program, shader->vertex);
  table->glDetachShader(shader->program, shader->fragment);
  table->glDeleteProgram(shader->program);
  table->glDeleteShader(shader->vertex);
  table->glDeleteShader(shader->fragment);
}
