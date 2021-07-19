#ifndef POOL_H
#define POOL_H

#include <stdbool.h>

// returns capacity required to store n objects of type in pool
#define POOL_CAPACITY(type, n) (n * sizeof(type) + (n + 7) / 8)

typedef struct Pool {
  char* memory;  // pointer to start of memory block
  int capacity;  // size of memory block

  int object_size; // size of object (including alignment)
  int n_objects;   // current number of objects stored in pool
  int max_objects; // maximum number of objects that this pool can contain

  void* free;      // pointer to the head of free list
} Pool;

// Initialize the pool
void pool_init(Pool* pool, char* memory, int capacity, int object_size, int alignment);
// Allocate memory for object in pool
// returns: pointer to allocated object, NULL if there is not enough memory
void* pool_aquire(Pool* pool);
// Return memory to the pool
void pool_release(Pool* pool, void* object);

// returns true if object belongs to the pool, i.e.
// 1. object is located in memory owned by pool
// 2. the alignment of object is correct
// 3. object at this location actually have been allocated via pool_aquire()
bool pool_contains(Pool* pool, void* object);
// returns a pointer to object at index
// NOTE: the object could be uninitialized
void* pool_at(Pool* pool, int index);
// returns index of object
// requires: pool_contains(object)
int pool_index(Pool* pool, void* object);
// returns number of objects allocated in the pool
int pool_size(Pool* pool);
// returns maximum number of objects that could be allocated in pool
int pool_capacity(Pool* pool);

// returns a pointer to the first object in pool
void* pool_first(Pool* pool);
// returns a pointer to the next object in pool
void* pool_next(Pool* pool, void* current);

#endif // POOL_H
