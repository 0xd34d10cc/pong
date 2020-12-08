#ifndef SHADER_H
#define SHADER_H

typedef struct VGL VGL;
typedef unsigned int ProgramID;

typedef struct Shader {
  ProgramID vertex;
  ProgramID fragment;

  ProgramID program;
} Shader;

int shader_compile(Shader* shader, VGL* table, const char* vertex_source, const char* fragment_source);
void shader_release(Shader* shader, VGL* table);

#endif // SHADER_H
