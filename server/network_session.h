#ifndef NETWORK_SESSION_H
#define NETWORK_SESSION_H

#include "config.h"

#include <netinet/in.h>

typedef struct Session Session;

enum {
  IO_EVENT_READ = (1 << 0),
  IO_EVENT_WRITE = (1 << 1)
} IoEvent;

typedef struct NetworkSession {
  int socket;
  unsigned events;

  char input[NET_BUFFER_SIZE];
  int received;

  char output[NET_BUFFER_SIZE];
  int to_send;

  struct sockaddr_in address;

  Session* game;
} NetworkSession;

void network_session_init(NetworkSession* session, int socket, struct sockaddr_in* address);
void network_session_close(NetworkSession* session);

#endif // NETWORK_SESSION_H
