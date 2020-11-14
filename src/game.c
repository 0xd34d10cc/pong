#include "game.h"

static int clamp(int x, int min, int max) {
  if (x < min) return min;
  if (x > max) return max;
  return x;
}

static bool in_range(int x, int left, int right) {
  return !(x < left || x > right);
}

static const int PLAYER_SPEED = 5;
static const int BALL_SPEED = PLAYER_SPEED - 2;

void game_init(Game* game, int board_width, int board_height) {
  game->state = RUNNING;
  game->player_pos = board_width / 2;
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
  *player = game->player_pos;
  *ball_x = game->ball_x;
  *ball_y = game->ball_y;
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
      if (game->state == LOST) {
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

  if (game->state == LOST) {
      return;
  }

  // update state
  int ball_right_bound = game->board_width - BALL_WIDTH;
  int ball_low_bound = game->board_height - BALL_HEIGHT;

  game->player_pos = clamp(game->player_pos + game->player_dx, 0, game->board_width - PLAYER_WIDTH);
  game->ball_x = clamp(game->ball_x + game->ball_dx, 0, ball_right_bound);
  game->ball_y = clamp(game->ball_y + game->ball_dy, 0, ball_low_bound);

  // floor/wall hit handling
  if (game->ball_x <= 0 || game->ball_x >= ball_right_bound) {
    game->ball_dx = -game->ball_dx;
  }

  if (game->ball_y <= 0 || game->ball_y >= ball_low_bound) {
    game->ball_dy = -game->ball_dy;

    if (game->ball_y >= ball_low_bound) {
      game->state = LOST;
    }
  }

  int low_bound_y = game->ball_y + BALL_HEIGHT;

  // player hit
  int player_y = game->board_height - PLAYER_HEIGHT;
  if (in_range(low_bound_y, player_y, player_y + game->ball_dy) &&
      in_range(game->ball_x,
        game->player_pos - PLAYER_WIDTH / 2 - BALL_WIDTH,
        game->player_pos + PLAYER_WIDTH / 2 + BALL_WIDTH)) {
    game->ball_dy = -game->ball_dy;
    game->ball_dy < 0 ? game->ball_dy-- : game->ball_dy++;
    game->ball_dx < 0 ? game->ball_dx-- : game->ball_dx++;

    if ((game->player_dx < 0) && (game->ball_dx > 0)) {
      game->ball_dx = -game->ball_dx;
    }

    if ((game->player_dx > 0) && (game->ball_dx < 0)) {
      game->ball_dx = -game->ball_dx;
    }
  }
}