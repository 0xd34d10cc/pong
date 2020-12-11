#include "game.h"
#include "bool.h"

#define HIT_SPEED_INC 0.005

static float clamp(float x, float min, float max) {
  if (x < min) return min;
  if (x > max) return max;
  return x;
}

static bool in_range(float x, float left, float right) {
  return !(x < left || x > right);
}

static const float PLAYER_SPEED = 0.015;
static const float BALL_SPEED = PLAYER_SPEED/2;

void game_init(Game* game, int board_width, int board_height) {
  game->state = STATE_RUNNING;
  game->player_x = 0;
  game->player_dx = 0;
  game->ball_x = 0;
  game->ball_y = 0;
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
  float ball_right_bound = 1 - BALL_WIDTH_COEF;
  float ball_low_bound = 1 - BALL_HEIGHT_COEF;

  game->player_x = clamp(
    game->player_x + game->player_dx,
    -1,
    1 - PLAYER_WIDTH_COEF
  );
  game->ball_x = clamp(game->ball_x + game->ball_dx, -1, 1 - BALL_WIDTH_COEF);
  game->ball_y = clamp(game->ball_y + game->ball_dy, -1, 1 - BALL_HEIGHT_COEF);

  if (game->ball_y >= ball_low_bound) {
    game->state = STATE_LOST;
  }

  if (game->ball_y <= -1) {
    game->ball_dy = -game->ball_dy;
  }

  // floor/wall collisions
  if (game->ball_x <= -1 || game->ball_x >= ball_right_bound) {
    game->ball_dx = -game->ball_dx;
  }
  // player bar collisioon
  float ball_bottom = game->ball_y + BALL_HEIGHT_COEF;
  float player_top = 1 - PLAYER_HEIGHT_COEF;

  LOG_INFO("player_top: %f, ball_bottom: %f, ball_x: %f, ball_y: %f", player_top, ball_bottom, game->ball_x, game->ball_y);
  LOG_INFO("player_x: %f, player_right: %f", game->player_x, game->player_x + PLAYER_WIDTH_COEF);
  if (in_range(ball_bottom, player_top, player_top + game->ball_dy) &&
      in_range(game->ball_x,
        game->player_x - BALL_WIDTH_COEF,
        game->player_x + PLAYER_WIDTH_COEF + BALL_WIDTH_COEF)) {
    game->ball_dy = -game->ball_dy;
    if (game->ball_dy < 0) {
      game->ball_dy -= HIT_SPEED_INC;
    } else {
      game->ball_dy += HIT_SPEED_INC;
    }

    if (game->ball_dx < 0) {
      game->ball_dx -= HIT_SPEED_INC;
    } else {
      game->ball_dx += HIT_SPEED_INC;
    }

    if ((game->player_dx != 0) && (game->player_dx > 0) != (game->ball_dx > 0)) {
      game->ball_dx = -game->ball_dx;
    }
  }
}
