#ifndef NETWORK_SESSION_H
#define NETWORK_SESSION_H

#include "config.h"

#include <netinet/in.h>

typedef struct Session Session;

typedef struct NetworkSession {
  int socket;

  char input[NET_BUFFER_SIZE];
  int received;

  char output[NET_BUFFER_SIZE];
  int sent;

  struct sockaddr_in address;

  Session* game;
} NetworkSession;

void network_session_init(NetworkSession* session, int socket, struct sockaddr_in* address);
void network_session_close(NetworkSession* session);

#endif // NETWORK_SESSION_H
