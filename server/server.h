#ifndef SERVER_H
#define SERVER_H

#ifndef WIN32
#include <stdatomic.h>
#include <netinet/in.h>
#include "net/reactor.h"
#else 
#define _CRT_SECURE_NO_WARNINGS
#include "net/reactor.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include "net/tcp_stream.h"
#include "net/tcp_listener.h"
#include "game/protocol.h"
#include "game/game.h"
#include "pool.h"


#define MAX_CONNECTIONS 32
#define MAX_LOBBIES 16

typedef struct Lobby Lobby;

typedef struct {
  // Client IO state
  // WARNING: must be first
  TcpStream stream;
  // ip and port of the client
  struct sockaddr_in address;
  Lobby* lobby;
} Connection;

typedef struct Lobby {
  Connection* owner;
  Connection* guest;

  char password[MAX_PASSWORD_SIZE];
  Game game;
} Lobby;

typedef struct {
#ifndef WIN32
  atomic_bool running;
#else
  LONG running;
#endif
  Reactor reactor;
  TcpListener listener;

  Evented timer;

  char connections_memory[POOL_CAPACITY(Connection, MAX_CONNECTIONS)];
  Pool connections;

  char lobbies_memory[POOL_CAPACITY(Lobby, MAX_LOBBIES)];
  Pool lobbies;
} Server;

int server_init(Server* server, const char* host, unsigned short port);
int server_run(Server* server);
void server_stop(Server* server);
void server_close(Server* server);

#endif // SERVER_H
