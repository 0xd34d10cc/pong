#ifndef PONG_H
#define PONG_H

#include "net/reactor.h"
#include "net/tcp_stream.h"
#include "game/game.h"
#include "renderer.h"

typedef struct SDL_Window SDL_Window;

typedef enum GameMode {
  LOCAL_GAME = 0,
  REMOTE_NEW_GAME,
  REMOTE_CONNECT_GAME
} GameMode;

typedef struct LaunchParams {
  // TODO: find max password size
  char password[100];
  int port;
  char ip[16];
  int game_mode;
  int session_id;
} LaunchParams;

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
  // Have not joined/created lobby yet
  NOT_IN_LOBBY = 0,
  // Lobby create message is sent, but no answer yet
  WAITING_FOR_LOBBY,
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
  // Window in which the game is running
  SDL_Window* window;
  // Renderer context
  Renderer renderer;
  // State of the game itself (board, players, ball, etc)
  Game game;

  // Reactor to poll for events
  Reactor reactor;
  // State of the connection
  ConnectionState connection_state;
  // Connection to the game server
  TcpStream tcp_stream;
  // State of the remote game session
  GameSession game_session;
} Pong;

// returns 0 on success
int pong_init(Pong* pong, const char* ip, unsigned short port, GameMode mode);
void pong_run(Pong* pong);
void pong_close(Pong* pong);

#endif // PONG_H
