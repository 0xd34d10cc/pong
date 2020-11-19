#include "pool.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "bool.h"


void pool_init(Pool* pool, int object_size) {
  assert(object_size >= sizeof(void*));
  pool->size = 0;
  pool->entry_size = object_size;
  pool->free = pool->buffer;
  int pool_max_size = POOL_CAPACITY / object_size;

  for (int i = 0; i < pool_max_size - 1; ++i) {
    int offset = pool->entry_size * i;
    char * start = pool->buffer + offset;
    char * end = pool->buffer + offset + pool->entry_size;
    memcpy(start, &end, sizeof (void*));
  }
  void* nullptr = NULL;

  void* last_obj = pool->buffer + object_size * (pool_max_size - 1);
  memcpy(last_obj, &nullptr, sizeof (void*));
}

void* pool_aquire(Pool* pool) {
  void* entry = pool->free;

  if (entry == NULL) {
    // no capacity
    return NULL;
  }

  pool->size++;
  memcpy(&pool->free, entry, sizeof(pool->free));
  return entry;
}

static bool pool_is_slot_taken(Pool* pool, int slot) {
  void* entry = pool->buffer + pool->entry_size * slot;

  // FIXME: do that in O(1) using bitvec
  void* head = pool->free;
  while (head != NULL) {
    if (head == entry) {
      return false;
    }

    void* next;
    memcpy(&next, head, sizeof(next));
    head = next;
  }

  return true;
}

void* pool_at(Pool* pool, int index) {
  if (index < 0 || index > POOL_CAPACITY / pool->entry_size || !pool_is_slot_taken(pool, index)) {
    return NULL;
  }

  return pool->buffer + pool->entry_size * index;
}

void pool_release(Pool* pool, void* object) {
  memcpy(object, &pool->free, sizeof(void*));
  pool->free = object;
  pool->size--;
}

int pool_index(Pool* pool, void* object) {
  return ((char*)object - pool->buffer) / pool->entry_size;
}

int pool_size(Pool* pool) {
  return pool->size;
}
