#include "game.h"

#include <stdbool.h>

#define HIT_SPEED_INC 0.005

static float clamp(float x, float min, float max) {
  if (x < min) return min;
  if (x > max) return max;
  return x;
}

static const float PLAYER_SPEED = 0.015;
static const float BALL_SPEED = PLAYER_SPEED/2;

void game_init(Game* game, bool is_multiplayer) {
  game->state = STATE_RUNNING;
  game->is_multiplayer = is_multiplayer;

  game->player = (GameObject) {
    .bbox = { .position = {0.0, -1.0}, .size = {PLAYER_WIDTH, PLAYER_HEIGHT } },
    .speed = { 0.0, 0.0 },
    .texture = TEXTURE_WHITE
  };

  game->opponent = (GameObject) {
    .bbox = { .position = {0.0, 1.0}, .size = {PLAYER_WIDTH, PLAYER_HEIGHT } },
    .speed = { 0.0, 0.0 },
    .texture = TEXTURE_WHITE
  };

  game->ball = (GameObject) {
    .bbox = { .position = {0.0, 0.0}, .size = {BALL_WIDTH, BALL_HEIGHT } },
    .speed = { BALL_SPEED, BALL_SPEED },
    .texture = TEXTURE_WHITE
  };

  game->board = (GameObject) {
    .bbox = {.position = {-1.0, -1.0}, .size = { 2.0, 2.0 } },
    .speed = { 0.0, 0.0 },
    .texture = TEXTURE_BLACK
  };
}

GameState game_state(Game* game) {
  return game->state;
}

void game_event(Game* game, Event event) {
  switch (event) {
    case EVENT_MOVE_LEFT:
      game->player.speed.x = -PLAYER_SPEED;
      break;
    case EVENT_MOVE_RIGHT:
      game->player.speed.x = PLAYER_SPEED;
      break;
    case EVENT_RESTART:
      if (game->state != STATE_RUNNING) {
          game_init(game, game->is_multiplayer);
      }
      break;
  }
}

void game_step_begin(Game* game) {
    game->player.speed.x = 0;
}

void game_set_player_speed(Game* game, Vec2 speed) {
  game->player.speed = speed;
}

void game_update_player_position(Game* game) {
  game->player.bbox.position = vec2_add(game->player.bbox.position, game->player.speed);
  rect_clamp(&game->player.bbox, &game->board.bbox);

  game->opponent.bbox.position = vec2_add(game->opponent.bbox.position, game->opponent.speed);
  rect_clamp(&game->opponent.bbox, &game->board.bbox);
}

// TODO: Rewrite all rectangles + speed to something like GameObject {Rectangle; Speed}
static void process_player_hit(Game* game, Rectangle* player, Vec2 player_speed) {
  bool curr_intersect = rect_intersect(&game->ball.bbox, player);
  Rectangle next_frame_ball = game->ball.bbox;
  next_frame_ball.position = vec2_add(next_frame_ball.position, game->ball.speed);
  bool next_intersect = rect_intersect(&next_frame_ball, player);

  if (curr_intersect || next_intersect) {
    game->ball.speed.y = -game->ball.speed.y;
    if (game->ball.speed.y  < 0) {
      game->ball.speed.y -= HIT_SPEED_INC;
    } else {
      game->ball.speed.y += HIT_SPEED_INC;
    }

    if (game->ball.speed.x < 0) {
      game->ball.speed.x -= HIT_SPEED_INC;
    } else {
      game->ball.speed.x += HIT_SPEED_INC;
    }

    if ((player_speed.x != 0) && (player_speed.x > 0) != (game->ball.speed.x > 0)) {
      game->ball.speed.x = -game->ball.speed.x;
    }
  }
}

void game_update_ball_position(Game* game) {
  game->ball.bbox.position = vec2_add(game->ball.bbox.position, game->ball.speed);
  if (game->ball.bbox.position.y < game->board.bbox.position.y) {
    game->state = STATE_LOST;
  }

  if (game->ball.bbox.position.y > game->board.bbox.position.y + game->board.bbox.size.y) {
    if (!game->is_multiplayer) {
      game->ball.speed.y = -game->ball.speed.y;
    }
    else {
      game->state = STATE_WON;
    }
  }

  // floor/wall collisions
  if (game->ball.bbox.position.x <= game->board.bbox.position.x ||
      game->ball.bbox.position.x + game->ball.bbox.size.x >= game->board.bbox.position.x + game->board.bbox.size.x) {
    game->ball.speed.x = -game->ball.speed.x;
  }

  process_player_hit(game, &game->player.bbox, game->player.speed);
  process_player_hit(game, &game->opponent.bbox, game->opponent.speed);
}


void game_step_end(Game* game, int ms) {
  // fixme: the code assumes ms == 16
  (void)ms;

  if (game->state == STATE_LOST || game->state == STATE_WON) {
      // no game logic for this state
      return;
  }

  game_update_player_position(game);

  game_update_ball_position(game);

}
