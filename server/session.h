#ifndef SESSION_H
#define SESSION_H

#include "game.h"
#include "config.h"

// Status that indicate status of inner server
typedef enum {
  // 1 Player has connected, waiting for another
  SESSION_STATE_CREATED,
  // 2 Player has connected, waiting for start
  SESSION_STATE_WAITING_FOR_PLAYER,
  // Playing
  SESSION_STATE_PLAYING,
  // Closed by any of players
  SESSION_STATE_PLAYER1_WON,
  SESSION_STATE_PLAYER2_WON
} SessionState;

typedef struct NetworkSession NetworkSession;

// TODO: rename this sturct to Lobby and update all related enums & comments
typedef struct Session {
  NetworkSession* player1;
  NetworkSession* player2;
  char password[MAX_PASSWORD_SIZE];

  SessionState state;
  Game game;
} Session;

void session_init(Session* session, NetworkSession* owner, const char* password);
void session_close(Session* session);

#endif // SESSION_H
