#ifndef SERVER_H
#define SERVER_H

#include "pool.h"

typedef struct Server {
  int master_socket;
  int poll;

  Pool connections;
  Pool sessions;
} Server;

int server_init(Server* server, const char* host, unsigned short port);
int server_run(Server* server);
void server_close(Server* server);


#endif // SERVER_H
