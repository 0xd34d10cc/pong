#ifndef NETWORK_SESSION_H
#define NETWORK_SESSION_H

#include "config.h"

typedef struct NetworkSession {
  int socket;
  unsigned events;

  char input[NET_BUFFER_SIZE];
  int received;

  char output[NET_BUFFER_SIZE];
  int to_send;

} NetworkSession;

int network_session_init(NetworkSession* session);
void network_session_close(NetworkSession* session);

int network_session_connect(NetworkSession* session, const char* host, unsigned short port);

#endif // NETWORK_SESSION_H