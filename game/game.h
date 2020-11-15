#ifndef GAME_H
#define GAME_H

#define PLAYER_WIDTH 100
#define PLAYER_HEIGHT 15

#define BALL_WIDTH 30
#define BALL_HEIGHT 30

// FIXME: these are client only definitions
#define DEFAULT_WINDOW_WIDTH 800
#define DEFAULT_WINDOW_HEIGHT 600

typedef int bool;
#define true 1
#define false 0

typedef struct Game {
  int state;

  int player_x;
  int player_dx;

  int ball_x;
  int ball_y;

  int ball_dx;
  int ball_dy;

  int board_width;
  int board_height;
} Game;

typedef enum {
  STATE_RUNNING,
  STATE_LOST
} GameState;

typedef enum {
  EVENT_MOVE_LEFT,
  EVENT_MOVE_RIGHT,

  EVENT_RESTART
} Event;

void game_init(Game* game, int board_width, int board_height);
GameState game_state(Game* game);
void game_positions(Game* game, int* player, int* ball_x, int* ball_y);
void game_event(Game* game, Event event);
void game_step_begin(Game* game);
void game_step_end(Game* game, int ms);

#endif // GAME_H
