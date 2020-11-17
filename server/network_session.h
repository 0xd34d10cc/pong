#ifndef NETWORK_SESSION_H
#define NETWORK_SESSION_H

#define BUFSIZE 512

#include <netinet/in.h>

typedef struct NetworkSession {
  int socket;
  char buf[BUFSIZE];
  struct sockaddr_in sock_addr;
  int offset;
} NetworkSession;

#endif // NETWORK_SESSION_H
