#ifndef GAME_H
#define GAME_H

#define PLAYER_WIDTH 0.125
#define PLAYER_HEIGHT 0.05
#define BALL_WIDTH 0.05
#define BALL_HEIGHT 0.05

#include "object.h"

typedef struct Game {
  int state;
  bool is_multiplayer;

  GameObject player;
  GameObject opponent;
  GameObject ball;
  GameObject board;
} Game;

typedef enum {
  STATE_RUNNING,
  STATE_LOST,
  STATE_WON
} GameState;

typedef enum {
  EVENT_MOVE_LEFT,
  EVENT_MOVE_RIGHT,

  EVENT_RESTART
} Event;

void game_init(Game* game, bool is_multiplayer);
GameState game_state(Game* game);
void game_event(Game* game, Event event);
void game_step_begin(Game* game);
void game_update_player_position(Game* game);
void game_update_ball_position(Game* game);
void game_step_end(Game* game, int ms);

#endif // GAME_H
