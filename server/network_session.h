#ifndef NETWORK_SESSION_H
#define NETWORK_SESSION_H

#define BUFSIZE 512

#include <netinet/in.h>

typedef struct Session Session;

typedef struct NetworkSession {
  int socket;

  char input[BUFSIZE];
  int received;

  char output[BUFSIZE];
  int sent;

  struct sockaddr_in sock_addr;

  Session* game;
} NetworkSession;

#endif // NETWORK_SESSION_H
