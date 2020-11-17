#ifndef POOL_H
#define POOL_H

#include "network_session.h"

#define POOL_MAX_CAPACITY 4096

// typedef union PoolEntry {
//  Session session;
//  union PoolEntry* next;
//} PoolEntry;

// A simple object pool implemented via statically sized buffer and free list
// WARNING: this object is not relocatable
// typedef struct SessionPool {
//  PoolEntry entries[POOL_MAX_SIZE];
//  int size;
//  PoolEntry* free;
//} SessionPool;

typedef struct ObjectPool {
  char buffer[POOL_MAX_CAPACITY];
  int entry_size;
  int size;
  void* free;
} ObjectPool;

void pool_init(ObjectPool* pool, int object_size);
void pool_release(ObjectPool* pool, void* object);
void* pool_aquire(ObjectPool* pool);

int pool_index(ObjectPool* pool, void* object);
int pool_size(ObjectPool* pool);


#endif // POOL_H
