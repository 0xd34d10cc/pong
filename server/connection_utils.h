#ifndef CONNECTION_UTILS_H
#define CONNECTION_UTILS_H

#include "messages.h"

// Status that indicate status of inner server
typedef enum SessionStatus {
  // 1 Player has connected, waiting for another
  Created = 0,
  // 2 Player has connected, waiting for start
  Pending = 1,
  // Playing
  InProgress = 2,
  // Closed by any of players
  Closed = 3,
  // Invalid status
  Invalid = 4
} SessionStatus;

// Status that we provide to end users about connection success or failure
typedef enum ClientStatus {
  // Successfully connected to the game session
  Connected = 0,
  // Provided Wrong session ID
  WrongSessionId = 1,
  // Provided Wrong password for existing session
  WrongPassword = 2,

  // Invalid status means that something went wrong
  InvalidStatus = 3
} ClientStatus;

typedef struct ConnectionStorage {
  int player1_sock;
  int player2_sock;
  int pw_size;
  char pw[PWDEFAULTSIZE];
  enum SessionStatus status;
} ConnectionStorage;

typedef struct ConnectionMap {
  int id;
  struct ConnectionStorage connection_storage;
  struct ConnectionMap* left;
  struct ConnectionMap* right;
} ConnectionMap;

void insert(struct ConnectionMap* map, int id, struct ConnectionStorage* connect);

struct ConnectionStorage* get_storage(struct ConnectionMap* map, int id);

void map_destroy(struct ConnectionMap* map);

#endif // CONNECTION_UTILS_H
