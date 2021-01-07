#ifndef SHADER_H
#define SHADER_H

#include "types.h"

typedef struct Shader {
  ProgramID vertex;
  ProgramID fragment;

  ProgramID program;
} Shader;

int shader_compile(Shader* shader, const char* vertex_source, const char* fragment_source);
void shader_release(Shader* shader);

void shader_bind(Shader* shader);
void shader_unbind(Shader* shader);

AttributeID shader_var(Shader* shader, const char* name);
AttributeID shader_uniform(Shader* shader, const char* name);

#endif // SHADER_H
