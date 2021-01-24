#include "game.h"

#include <stdbool.h>

#define HIT_SPEED_INC 0.00025

static float clamp(float x, float min, float max) {
  if (x < min) return min;
  if (x > max) return max;
  return x;
}

struct GameObject {
  Rectangle bbox;
  Vec2 speed;
}

static float get_slope(Vec2 start, Vec2 end) {
  float diff_y = end.y - start.y;
  float diff_x = end.x - start.x;

  return diff_y/diff_x;
}

static Vec2 find_intersect_point(Vec2 v11, Vec2 v12, Vec2 v21, Vec v22) {
  //  {
  //    y - y11 = k1 (x - x11)
  //    y - y21 = k2 (x - x21)
  //  }

  // y = k1 (x - x11) + y11
  // y = k2 (x - x21) + y21

  // 0 = (k1 (x - x11) + y11) - (k2 (x - x21) + y21)
  // y = k2 (x - x21) + y21

  // 0 = (k1*x - k1*x11 + y11) - (k2*x - k2*x21 + y21)
  // y = k2 (x - x21) + y21

  // x = (k1 * x11 - k2 * x21 + y21 - y11) / (k1 - k2)  ????
  // y = k2 (x - x21) + y21

  float slope1 = get_slope(v11, v12);
  float slope2 = get_slope(v21, v22);
}


static float find_intersection_time(GameObject player, GameObject ball, float dt) {
  const float player_top_ordinate = player.bbox.position.y + player.bbox.size.y;
  // y2 = y1 + v1 * t
  // t = (y2 - y1) / v1
  const float ordinate_intersection_time = (player_top_ordinate - ball.bbox.position.y) / ball.speed.y;
  if (ordinate_intersection_time > dt) {
    return -1.0;
  }

  const Vec2 ball_movement = vec2_mul(ball.speed, ordinate_intersection_time);
  const Vec2 ball_position_at_intersection = vec2_add(ball.bbox.position, ball_movement);

  const Vec2 player_movement = vec2_mul(player.speed, ordinate_intersection_time);
  const Vec2 player_position_at_intersection = vec2_add(player.bbox.position, player_movement);

  
}

// speed in unit/ms
static const float PLAYER_SPEED = 0.0009;
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
  if (game->state == STATE_LOST || game->state == STATE_WON) {
      // no game logic for this state
      return;
  }

  game_update_player_position(game, ms);

  game_update_ball_position(game, ms);
}
