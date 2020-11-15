#include "pool.h"

#include <stddef.h>

void pool_init(SessionPool* pool) {
  pool->size = 0;
  pool->free = &pool->entries[0];
  for (int i = 0; i < POOL_MAX_SIZE - 1; ++i) {
    pool->entries[i].next = &pool->entries[i + 1];
  }
  pool->entries[POOL_MAX_SIZE - 1].next = NULL;
}

Session* pool_aquire(SessionPool* pool) {
  PoolEntry* entry = pool->free;
  if (entry == NULL) {
    // no capacity
    return NULL;
  }

  pool->size++;
  pool->free = entry->next;
  return &entry->session;
}

void pool_release(SessionPool* pool, Session* session) {
  PoolEntry* entry = (PoolEntry*)session;
  entry->next = pool->free;
  pool->free = entry;
  pool->size--;
}

int pool_index(SessionPool* pool, Session* session) {
  PoolEntry* entry = (PoolEntry*)session;
  return entry - &pool->entries[0];
}

int pool_size(SessionPool* pool) {
  return pool->size;
}
