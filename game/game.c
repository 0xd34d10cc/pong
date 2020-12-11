#include "game.h"
#include "bool.h"

static int clamp(int x, int min, int max) {
  if (x < min) return min;
  if (x > max) return max;
  return x;
}

static bool in_range(int x, int left, int right) {
  return !(x < left || x > right);
}

static const int PLAYER_SPEED = 0.1;
static const int BALL_SPEED = PLAYER_SPEED - 0.05;

void game_init(Game* game, int board_width, int board_height) {
  game->state = STATE_RUNNING;
  game->player_x = board_width / 2 - PLAYER_WIDTH / 2;
  game->player_dx = 0;
  game->ball_x = board_width / 2;
  game->ball_y = board_height / 2;
  game->ball_dx = BALL_SPEED;
  game->ball_dy = BALL_SPEED;
  game->board_width = board_width;
  game->board_height = board_height;
}

GameState game_state(Game* game) {
  return game->state;
}

void game_positions(Game* game, int* player, int* ball_x, int* ball_y) {
  *player = (game->player_x + 1) * (game->board_width /2);
  *ball_x = (game->ball_x + 1) * (game->board_width / 2);
  *ball_y = (game->ball_y +1) * (game->board_height /2 );
}

void game_event(Game* game, Event event) {
  switch (event) {
    case EVENT_MOVE_LEFT:
      game->player_dx = -PLAYER_SPEED;
      break;
    case EVENT_MOVE_RIGHT:
      game->player_dx = PLAYER_SPEED;
      break;
    case EVENT_RESTART:
      if (game->state == STATE_LOST) {
          game_init(game, game->board_width, game->board_height);
      }
      break;
  }
}

void game_step_begin(Game* game) {
    game->player_dx = 0;
}

void game_step_end(Game* game, int ms) {
  // fixme: the code assumes ms == 16
  (void)ms;

  if (game->state == STATE_LOST) {
      // no game logic for this state
      return;
  }

  // update state
  int ball_right_bound = game->board_width - BALL_WIDTH;
  int ball_low_bound = game->board_height - BALL_HEIGHT;

  game->player_x = clamp(
    game->player_x + game->player_dx,
    0,
    game->board_width - PLAYER_WIDTH
  );
  game->ball_x = clamp(game->ball_x + game->ball_dx, 0, ball_right_bound);
  game->ball_y = clamp(game->ball_y + game->ball_dy, 0, ball_low_bound);

  if (game->ball_y >= ball_low_bound) {
    game->state = STATE_LOST;
  }

  if (game->ball_y <= 0) {
    game->ball_dy = -game->ball_dy;
  }

  // floor/wall collisions
  if (game->ball_x <= 0 || game->ball_x >= ball_right_bound) {
    game->ball_dx = -game->ball_dx;
  }

  // player bar collisioon
  int ball_bottom = game->ball_y + BALL_HEIGHT;
  int player_top = game->board_height - PLAYER_HEIGHT;
  if (in_range(ball_bottom, player_top, player_top + game->ball_dy) &&
      in_range(game->ball_x,
        game->player_x - BALL_WIDTH,
        game->player_x + PLAYER_WIDTH + BALL_WIDTH)) {
    game->ball_dy = -game->ball_dy;
    game->ball_dy < 0 ? game->ball_dy-- : game->ball_dy++;
    game->ball_dx < 0 ? game->ball_dx-- : game->ball_dx++;

    if ((game->player_dx != 0) && (game->player_dx > 0) != (game->ball_dx > 0)) {
      game->ball_dx = -game->ball_dx;
    }
  }
}