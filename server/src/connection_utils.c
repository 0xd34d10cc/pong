#include <stdlib.h>

#include "connection_utils.h"

void insert(struct ConnectionMap* map, int id, struct ConnectionStorage* connect) {    
  if (map->id == id) {    
      return;    
  }     
  if (map->id < id) {    
      if (map->right) insert(map->right, id, connect);    
      else {    
        struct ConnectionMap * new_map = malloc(sizeof(ConnectionMap));    
        new_map->connection_storage = *connect;    
        new_map->id = id;    
        map->right = new_map;    
        return;    
      }    
  }    
  if (map->id > id) {    
    if(map->left) insert(map->left, id, connect);    
    else {    
      struct ConnectionMap * new_map = malloc(sizeof(ConnectionMap));    
      new_map->connection_storage = *connect;    
      new_map->id = id;    
      map->left = new_map;    
      return;    
      }    
  }    
}    


