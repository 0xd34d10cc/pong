#ifndef PONG_H
#define PONG_H

#include "renderer.h"
#include "reactor.h"
#include "network_session.h"
#include "game.h"

typedef struct SDL_Window SDL_Window;

enum {
  LOCAL = 0,
  AWAITING_CONNECTION,
  CONNECTED,
  DISCONNECTED
};

typedef struct ConnectionState {
  int state;
  char ip[16];
  unsigned short port;
} ConnectionState;

typedef struct Pong {
  bool running;
  SDL_Window* window;
  Renderer renderer;
  Game game;

  Reactor reactor;
  ConnectionState connection_state;
  NetworkSession network_session;
} Pong;

// returns 0 on success
int pong_init(Pong* pong, const char* ip, unsigned short port);
void pong_run(Pong* pong);
void pong_close(Pong* pong);

#endif // PONG_H