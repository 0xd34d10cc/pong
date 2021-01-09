#include "buffer.h"

void vertex_buffer_init(VertexBuffer* buffer, size_t size) {
  ObjectID id;
  vgl.glGenBuffers(1, &id);

  buffer->id = id;
  buffer->size = size;

  vertex_buffer_bind(buffer);
  vgl.glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STREAM_DRAW);
}

void vertex_buffer_release(VertexBuffer* buffer) {
  vgl.glDeleteBuffers(1, &buffer->id);
}

void vertex_buffer_bind(VertexBuffer* buffer) {
  vgl.glBindBuffer(GL_ARRAY_BUFFER, buffer->id);
}

void vertex_buffer_unbind(VertexBuffer* buffer) {
  (void)buffer;
  vgl.glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void* vertex_buffer_map(VertexBuffer* buffer) {
  (void)buffer;
  return vgl.glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
}

void vertex_buffer_unmap(VertexBuffer* buffer) {
  (void)buffer;
  vgl.glUnmapBuffer(GL_ARRAY_BUFFER);
}

// ==============================================================

void index_buffer_init(IndexBuffer* buffer, size_t size) {
  ObjectID id;
  vgl.glGenBuffers(1, &id);

  buffer->id = id;
  buffer->size = size;

  index_buffer_bind(buffer);
  vgl.glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, NULL, GL_STREAM_DRAW);
}

void index_buffer_release(IndexBuffer* buffer) {
  vgl.glDeleteBuffers(1, &buffer->id);
}

void index_buffer_bind(IndexBuffer* buffer) {
  vgl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->id);
}

void index_buffer_unbind(IndexBuffer* buffer) {
  (void)buffer;
  vgl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void* index_buffer_map(IndexBuffer* buffer) {
  (void)buffer;
  return vgl.glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
}

void index_buffer_unmap(IndexBuffer* buffer) {
  (void)buffer;
  vgl.glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
}
