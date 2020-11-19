#ifndef POOL_H
#define POOL_H

#include "network_session.h"

#define POOL_MAX_CAPACITY 4096

typedef struct Pool {
  char buffer[POOL_MAX_CAPACITY];
  int entry_size;
  int size;
  void* free;
} Pool;

void pool_init(Pool* pool, int object_size);
void pool_release(Pool* pool, void* object);
void* pool_aquire(Pool* pool);

int pool_index(Pool* pool, void* object);
int pool_size(Pool* pool);


#endif // POOL_H
