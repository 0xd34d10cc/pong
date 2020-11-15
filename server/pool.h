#ifndef POOL_H
#define POOL_H

// FIXME: this should be in a separate file
typedef struct Session {
  int socket;
  char buffer[1024];
  // todo
} Session;

#define POOL_MAX_SIZE 2

typedef union PoolEntry {
  Session session;
  union PoolEntry* next;
} PoolEntry;

// A simple object pool implemented via statically sized buffer and free list
// WARNING: this object is not relocatable
typedef struct SessionPool {
  PoolEntry entries[POOL_MAX_SIZE];
  int size;
  PoolEntry* free;
} SessionPool;

void pool_init(SessionPool* pool);
Session* pool_aquire(SessionPool* pool);
void pool_release(SessionPool* pool, Session* session);
int pool_index(SessionPool* pool, Session* session);
int pool_size(SessionPool* pool);

#endif // POOL_H
