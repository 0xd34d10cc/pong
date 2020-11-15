#ifndef CONNECTION_UTILS_H
#define CONNECTION_UTILS_H

#include "messages.h"

// len(255.255.255.255:50000) - 21 + 1 (zero terminated code)    
struct ConnectionStorage {    
  char player1_addr[22];    
  char player2_addr[22];    
  int pw_size;     
  char pw[PWDEFAULTSIZE];    
} ConnectionStorage;    
    
struct ConnectionMap {    
  int id;    
  struct ConnectionStorage connection_storage;    
  struct ConnectionMap* left;    
  struct ConnectionMap* right;    
} ConnectionMap;    

void insert(struct ConnectionMap* map, int id, struct ConnectionStorage* connect);

#endif // CONNECTION_UTILS_H
