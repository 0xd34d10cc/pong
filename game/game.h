#ifndef GAME_H
#define GAME_H

#define PLAYER_WIDTH_COEF 0.125
#define PLAYER_HEIGHT_COEF 0.025
#define BALL_WIDTH_COEF 0.05
#define BALL_HEIGHT_COEF 0.05

// FIXME: these are client only definitions
#define DEFAULT_WINDOW_WIDTH 800
#define DEFAULT_WINDOW_HEIGHT 600

typedef struct Game {
  int state;

  float player_x;
  float player_dx;

  float ball_x;
  float ball_y;

  float ball_dx;
  float ball_dy;

  int board_width;
  int board_height;
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

void game_init(Game* game, int board_width, int board_height);
GameState game_state(Game* game);
void game_positions(Game* game, int* player, int* ball_x, int* ball_y);
void game_event(Game* game, Event event);
void game_step_begin(Game* game);
void game_step_end(Game* game, int ms);

#endif // GAME_H
