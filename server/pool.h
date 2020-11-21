#ifndef POOL_H
#define POOL_H

#define POOL_CAPACITY 4096

typedef struct Pool {
  char buffer[POOL_CAPACITY];
  int entry_size;  // size of object
  int size;        // number of object
  void* free;      // pointer to the head of free list
} Pool;

void pool_init(Pool* pool, int object_size);
void* pool_aquire(Pool* pool);
void* pool_at(Pool* pool, int index);
void pool_release(Pool* pool, void* object);

int pool_index(Pool* pool, void* object);
int pool_size(Pool* pool);

#endif // POOL_H
