#ifndef PONG_H
#define PONG_H

#include "renderer.h"
#include "reactor.h"
#include "network_session.h"
#include "game.h"

typedef struct SDL_Window SDL_Window;

enum {
  // Local game session, without any networking
  LOCAL = 0,
  // Disconnected (or didn't connected yet) from game server
  DISCONNECTED,
  // Connect is triggered, but we still waiting for connection to server
  AWAITING_CONNECTION,
  // Connected to the game server
  CONNECTED
};

enum {
  // Game session is not yet created
  NOT_IN_SESSION = 0,
  // Game session is created, but no second player here
  CREATED,
  // We joined to another session, or someone is joined to our session
  JOINED,
  // Playing pong with another player
  PLAYING
};

typedef struct GameSession {
  int id;
  int state;
  char opponent_ip[16];
} GameSession;

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
  GameSession game_session;
} Pong;

// returns 0 on success
int pong_init(Pong* pong, const char* ip, unsigned short port);
void pong_run(Pong* pong);
void pong_close(Pong* pong);

#endif // PONG_H
