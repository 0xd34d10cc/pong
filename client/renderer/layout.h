#ifndef RENDERER_LAYOUT_H
#define RENDERER_LAYOUT_H

#define LAYOUT_MAX_FIELDS 16

#include "types.h"

typedef enum {
  LAYOUT_FIELD_TYPE_FLOAT,
  LAYOUT_FIELD_TYPE_BYTES
} FieldType;

typedef struct {
  AttributeID id;
  unsigned short n;
  FieldType type;
  bool normalized;
} VertexFieldDescription;

typedef struct {
  VertexFieldDescription fields[LAYOUT_MAX_FIELDS]; // description of each field
  unsigned short n;                                 // number of fields
  int total_size;                                   // total size of the vertex struct
} VertexLayout;

void layout_init(VertexLayout* layout);
void layout_float(VertexLayout* layout, AttributeID id, unsigned short count);
void layout_bytes(VertexLayout* layout, AttributeID id, unsigned short count);
void layout_set(VertexLayout* layout, ObjectID vertex_array);

#endif // RENDERER_LAYOUT_H
