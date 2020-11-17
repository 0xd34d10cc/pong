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


struct ConnectionStorage* get_storage(struct ConnectionMap* map, int id) {
  if (map->id == id) {
    return &map->connection_storage;
  }

  if (map->id < id) {
    if (map->right) {
      return get_storage(map->right, id);
    } else {
      return NULL;
    }
  }

  if (map->id > id) {
    if (map->left) {
      return get_storage(map->left, id);
    } else {
      return NULL;
    }
  }

  return NULL;
}

void map_destroy(struct ConnectionMap* map) {
  if (NULL == map) return;
  if (map->left) map_destroy(map->left);
  if (map->right) map_destroy(map->right);
  free(map);
}
