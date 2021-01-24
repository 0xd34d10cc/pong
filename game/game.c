#include "game.h"

#include <stdbool.h>

#define HIT_SPEED_INC 0.00025

static float clamp(float x, float min, float max) {
  if (x < min) return min;
  if (x > max) return max;
  return x;
}

static float find_intersection_time(GameObject player, GameObject ball, float dt) {
  const bool is_player_lower = ball.bbox.position.y > player.bbox.position.y;
  const float player_nearest_ordinate = is_player_lower ? player.bbox.position.y + player.bbox.size.y :
                                                          player.bbox.position.y;

  const float ball_nearest_ordinate = is_player_lower ? ball.bbox.position.y :
                                                        ball.bbox.position.y + ball.bbox.size.y;
  // y2 = y1 + v1 * t
  // t = (y2 - y1) / v1
  const float ordinate_intersection_time = (player_nearest_ordinate - ball_nearest_ordinate) / ball.speed.y;
  if (ordinate_intersection_time > dt || ordinate_intersection_time < 0.0) {
    return -1.0;
  }

  const Vec2 ball_movement = vec2_mul(ball.speed, ordinate_intersection_time);
  ball.bbox.position = vec2_add(ball.bbox.position, ball_movement);

  const Vec2 player_movement = vec2_mul(player.speed, ordinate_intersection_time);
  player.bbox.position = vec2_add(player.bbox.position, player_movement);

  if (!rect_intersect(&player.bbox, &ball.bbox)) {
    return -1.0;
  }

  return ordinate_intersection_time;
}

// speed in unit/ms
static const float PLAYER_SPEED = 0.0009;
static const float BALL_SPEED = PLAYER_SPEED/2;
static const float WALL_THICKNESS = 1.0;
static const float WALL_LENGTH = 2.0;

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

  game->walls[0].bbox.position = (Vec2) {-1.0, -1.0 - WALL_THICKNESS}; // Bottom wall
  game->walls[1].bbox.position = (Vec2) {-1.0 - WALL_THICKNESS, -1.0}; // Left wall
  game->walls[2].bbox.position = (Vec2) {-1.0, 1.0}; // Top wall
  game->walls[3].bbox.position = (Vec2) {1.0, -1.0}; // Right wall

  for(int wall_index = 0; wall_index < 4; ++wall_index) {
    game->walls[wall_index].speed = (Vec2) {0.0, 0.0};
    if(wall_index % 2 == 0) { // Top and Bottom walls sizes
      game->walls[wall_index].bbox.size = (Vec2) {WALL_LENGTH, WALL_THICKNESS};
    }
    else { // Left and Right walls sizes
      game->walls[wall_index].bbox.size = (Vec2) {WALL_THICKNESS, WALL_LENGTH};
    }
  }
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


void game_step_end(Game* game, float dt) {
  if (game->state == STATE_LOST || game->state == STATE_WON) {
      // no game logic for this state
      return;
  }

  while (game->state == STATE_RUNNING && dt > 0.0) {
    const float player_intersection_time = find_intersection_time(game->player, game->ball, dt);
    const float opponent_intersection_time = find_intersection_time(game->opponent, game->ball, dt);

    for(int i = 0; i < 4; ++i) {
       const float wall_intersection = 
    }
  }
}
