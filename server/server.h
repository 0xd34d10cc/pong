#ifndef SERVER_H
#define SERVER_H

typedef struct Server {
  int master_socket;
  int poll;

  ObjectPool connections;
  ObjectPool sessions;
} Server;

int server_init(Server* server, const char* host, unsigned short port);
int server_run(Server* server);
void server_close(Server* server);


#endif // SERVER_H
