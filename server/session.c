#include "session.h"

void session_init(Session* session, NetworkSession* owner, const char* password) {
  session->player1 = owner;
  session->player2 = NULL;
  session->state = SESSION_STATE_CREATED;
  strcpy(session->password, password);
  // FIXME: unhardcode the board size
  game_init(&session->game, 800, 600);
}

void session_close(Session* session) {
  // nothing, for now
}
