#include "layout.h"


void layout_init(VertexLayout* layout) {
  layout->n = 0;
  layout->total_size = 0;
}

static int field_size(FieldType type) {
  switch (type) {
    case LAYOUT_FIELD_TYPE_FLOAT:
      return 4;
    case LAYOUT_FIELD_TYPE_BYTES:
      return 1;
    default:
      LOG_FATAL("Invlaid field type: %d", type);
  }
}

static int translate_type(FieldType type) {
  switch (type) {
    case LAYOUT_FIELD_TYPE_FLOAT:
      return GL_FLOAT;
    case LAYOUT_FIELD_TYPE_BYTES:
      return GL_UNSIGNED_BYTE;
    default:
      LOG_FATAL("Invalid field type: %d", type);
  }
}

void layout_float(VertexLayout* layout, AttributeID id, unsigned short count) {
  if (layout->n >= LAYOUT_MAX_FIELDS) {
    LOG_FATAL("Unsupported number of fields for vertex layout");
  }

  FieldType type = LAYOUT_FIELD_TYPE_FLOAT;
  layout->fields[layout->n] = (VertexFieldDescription) {
    .id = id,
    .n = count,

    .type = type,
    .normalized = false
  };
  layout->total_size += field_size(type) * count;
  layout->n++;
}

void layout_bytes(VertexLayout* layout, AttributeID id, unsigned short count) {
  if (layout->n >= LAYOUT_MAX_FIELDS) {
    LOG_FATAL("Unsupported number of fields for vertex layout");
  }

  FieldType type = LAYOUT_FIELD_TYPE_BYTES;
  layout->fields[layout->n] = (VertexFieldDescription) {
    .id = id,
    .n = count,
    .type = type,
    .normalized = true
  };
  layout->total_size += field_size(type) * count;
  layout->n++;
}

void layout_set(VertexLayout* layout, ObjectID vertex_array) {
  vgl.glBindVertexArray(vertex_array);

  ptrdiff_t offset = 0;
  for (int i = 0; i < layout->n; ++i) {
    VertexFieldDescription* field = &layout->fields[i];
    vgl.glEnableVertexAttribArray(field->id);

    int type = translate_type(field->type);
    int normalized = field->normalized ? GL_TRUE : GL_FALSE;
    vgl.glVertexAttribPointer(field->id, field->n, type, normalized, layout->total_size, (const void*)offset);

    offset += field_size(field->type) * field->n;
  }
}
