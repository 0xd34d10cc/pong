#ifndef RENDERER_BUFFER_H
#define RENDERER_BUFFER_H

#include "types.h"
#include "vgl.h"

typedef struct {
  ObjectID id;
  size_t size;
} VertexBuffer;

void vertex_buffer_init(VertexBuffer* buffer, size_t size /* in bytes */);
void vertex_buffer_release(VertexBuffer* buffer);

void vertex_buffer_bind(VertexBuffer* buffer);
void vertex_buffer_unbind(VertexBuffer* buffer);

void* vertex_buffer_map(VertexBuffer* buffer);
void vertex_buffer_unmap(VertexBuffer* buffer);

typedef struct {
  ObjectID id;
  size_t size;
} IndexBuffer;

void index_buffer_init(IndexBuffer* buffer, size_t size /* also in bytes */);
void index_buffer_release(IndexBuffer* buffer);

void index_buffer_bind(IndexBuffer* buffer);
void index_buffer_unbind(IndexBuffer* buffer);

void* index_buffer_map(IndexBuffer* buffer);
void index_buffer_unmap(IndexBuffer* buffer);

#endif // RENDERER_BUFFER_H
