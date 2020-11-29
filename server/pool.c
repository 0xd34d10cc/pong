#include "pool.h"

#include <assert.h>
#include <stddef.h>
#include <stdalign.h>
#include <string.h>

#include "bool.h"

// obtain a pointer to start of "in use" bitmask
static unsigned char* pool_slots(Pool* pool) {
  return pool_at(pool, pool->max_objects);
}

static bool pool_slot_get(Pool* pool, int index) {
  unsigned char* slots = pool_slots(pool);
  return slots[index / 8] & (1 << (index % 8));
}

static void pool_slot_set(Pool* pool, int index, bool taken) {
  unsigned char* slots = pool_slots(pool);
  if (taken) {
    slots[index / 8] |= (1 << (index % 8));
  } else {
    slots[index / 8] &= ~(1 << (index % 8));
  }
}

void pool_init(Pool* pool, char* memory, int capacity, int object_size, int alignment) {
  assert((intptr_t)memory % alignment == 0);
  assert(object_size >= sizeof(void*));
  assert(alignment >= alignof(void*));

  pool->memory = memory;
  pool->capacity = capacity;

  pool->object_size = object_size;
  pool->n_objects = 0;
  // max_objects = capacity / (object_size + 1/8)
  pool->max_objects = (capacity * 8) / (object_size * 8 + 1);
  pool->free = pool_at(pool, 0);

  // initialize free list
  for (int i = 0; i < pool->max_objects - 1; ++i) {
    void* current = pool_at(pool, i);
    void* next = pool_at(pool, i + 1);
    *(void**)current = next;
  }

  void* last = pool_at(pool, pool->max_objects - 1);
  *(void**)last = NULL;

  // initialize "in use" mask
  unsigned char* slots = pool_slots(pool);
  memset(slots, 0, (pool->max_objects + 7) / 8);
}

void* pool_aquire(Pool* pool) {
  void* entry = pool->free;
  if (entry == NULL) {
    // no capacity
    return NULL;
  }

  pool->n_objects++;
  pool->free = *(void**)entry;
  pool_slot_set(pool, pool_index(pool, entry), true);
  assert(pool_contains(pool, entry));
  return entry;
}

void* pool_at(Pool* pool, int index) {
  return pool->memory + pool->object_size * index;
}

void pool_release(Pool* pool, void* object) {
  assert(pool_contains(pool, object));
  pool_slot_set(pool, pool_index(pool, object), false);
  *(void**)object = pool->free;
  pool->free = object;
  pool->n_objects--;
}

bool pool_contains(Pool* pool, void* object) {
  if (object < pool_at(pool, 0) || object > pool_at(pool, pool->max_objects - 1)) {
    return false;
  }

  int offset = (char*)object - (char*)pool_at(pool, 0);
  if (offset % pool->object_size != 0) {
    assert(false);
    return false;
  }

  return pool_slot_get(pool, pool_index(pool, object));
}

int pool_index(Pool* pool, void* object) {
  return ((char*)object - pool->memory) / pool->object_size;
}

int pool_size(Pool* pool) {
  return pool->n_objects;
}

int pool_capacity(Pool* pool) {
  return pool->max_objects;
}

static void* pool_search_forward(Pool* pool, int start) {
  for (int i = start; i < pool->max_objects; ++i) {
    if (pool_slot_get(pool, i)) {
      return pool_at(pool, i);
    }
  }

  return NULL;
}

void* pool_first(Pool* pool) {
  return pool_search_forward(pool, 0);
}

void* pool_next(Pool* pool, void* object) {
  return pool_search_forward(pool, pool_index(pool, object) + 1);
}
