#include "game.h"

#include <stdbool.h>
#include <math.h>
#include <assert.h>

#define HIT_SPEED_INC 0.00025

static float clamp(float x, float min, float max) {
  if (x < min) return min;
  if (x > max) return max;
  return x;
}

typedef struct {
  // IF no collision detected time equal to INFINITY
  float time;
  Vec2 normal;
  GameObject* what;
  GameObject* with_what;
} Collision;

static const Collision NO_COLLISION = {.time = INFINITY };

static void min_collision(Collision* lhs, Collision* rhs) {
  if(rhs->time < lhs->time) {
    memcpy(lhs, rhs, sizeof(Collision));
  }
}

static Collision find_player_collision(GameObject* player, GameObject* ball, float dt) {
  const bool is_player_lower = ball->bbox.position.y > player->bbox.position.y;
  const float player_nearest_ordinate = is_player_lower ? player->bbox.position.y + player->bbox.size.y :
                                                          player->bbox.position.y;

  const float ball_nearest_ordinate = is_player_lower ? ball->bbox.position.y :
                                                        ball->bbox.position.y + ball->bbox.size.y;
  // y2 = y1 + v1 * t
  // t = (y2 - y1) / v1
  const float ordinate_intersection_time = (player_nearest_ordinate - ball_nearest_ordinate) / ball->speed.y;
  if (ordinate_intersection_time > dt || ordinate_intersection_time < 0.0) {
    return NO_COLLISION;
  }

  GameObject player_temp = *player;
  GameObject ball_temp = *ball;
  const Vec2 ball_movement = vec2_mul(ball_temp.speed, ordinate_intersection_time);
  ball_temp.bbox.position = vec2_add(ball_temp.bbox.position, ball_movement);

  const Vec2 player_movement = vec2_mul(player_temp.speed, ordinate_intersection_time);
  player_temp.bbox.position = vec2_add(player_temp.bbox.position, player_movement);

  
  if (!rect_intersect(&player_temp.bbox, &ball_temp.bbox)) {
    return NO_COLLISION;
  }

  return (Collision) {
    .time = ordinate_intersection_time,
    .normal = vec2(0, is_player_lower ? 1.0 : -1.0),
    .what = ball,
    .with_what = player
  };
}

static Collision find_wall_collision(GameObject* ball, GameObject* wall, float dt) {
  const bool is_ball_moves_right = ball->speed.x > 0;
  const bool is_ball_moves_top = ball->speed.y > 0;

  float dx_entry;
  float dx_exit;
  if(is_ball_moves_right) {
    const float ball_x_entry = ball->bbox.position.x + ball->bbox.size.x;
    const float wall_x_entry = wall->bbox.position.x;
    dx_entry = wall_x_entry - ball_x_entry;

    const float ball_x_exit = ball->bbox.position.x;
    const float wall_x_exit = wall->bbox.position.x + wall->bbox.size.x;

    dx_exit = wall_x_exit - ball_x_exit;
  }
  else {
    const float ball_x_entry = ball->bbox.position.x;
    const float wall_x_entry = wall->bbox.position.x + wall->bbox.size.x;

    dx_entry = wall_x_entry - ball_x_entry;

    const float ball_x_exit = ball->bbox.position.x + ball->bbox.size.x;
    const float wall_x_exit = wall->bbox.position.x;

    dx_exit = wall_x_exit - ball_x_exit;
  }

  float dy_entry;
  float dy_exit;
  if(is_ball_moves_top) {
    const float ball_y_entry = ball->bbox.position.y + ball->bbox.size.y;
    const float wall_y_entry = wall->bbox.position.y;

    dy_entry = wall_y_entry - ball_y_entry;

    const float ball_y_exit = ball->bbox.position.y;
    const float wall_y_exit = wall->bbox.position.y + wall->bbox.size.y;

    dy_exit = wall_y_exit - ball_y_exit;
  }
  else {
    const float ball_y_entry = ball->bbox.position.y;
    const float wall_y_entry = wall->bbox.position.y + wall->bbox.size.y;

    dy_entry = wall_y_entry - ball_y_entry;

    const float ball_y_exit = ball->bbox.position.y + ball->bbox.size.y;
    const float wall_y_exit = wall->bbox.position.y;

    dy_exit = wall_y_exit - ball_y_exit;
  }

  const float x_entry_time = dx_entry / ball->speed.x;
  const float x_exit_time = dx_exit / ball->speed.x;

  const float y_entry_time = dy_entry / ball->speed.y;
  const float y_exit_time = dy_exit / ball->speed.y;

  const float entry_time = x_entry_time > y_entry_time ? x_entry_time : y_entry_time;
  const float exit_time = x_exit_time < y_exit_time ? x_exit_time : y_exit_time;

  if(entry_time > exit_time || x_entry_time < 0 && y_entry_time < 0 || 
    x_entry_time > dt || y_entry_time > dt) {
      return NO_COLLISION;
  }

  Vec2 normal;
  if(x_entry_time > y_entry_time) {
    normal.y = 0.0;
    normal.x = dx_entry < 0.0 ? 1.0 : -1.0;
  }
  else {
    normal.x = 0.0;
    normal.y = dy_entry < 0.0 ? 1.0 : -1.0;
  }

  return (Collision) {
    .time = entry_time,
    .normal = normal,
    .what = ball,
    .with_what = wall
  };
}

// speed in unit/ms
static const float PLAYER_SPEED = 0.001;
static const float BALL_SPEED = PLAYER_SPEED/2;
static const float WALL_THICKNESS = 1.0;
static const float WALL_LENGTH = 2.0;
static const float EPSILON = 0.001;

void game_init(Game* game, bool is_multiplayer) {
  game->state = STATE_RUNNING;
  game->is_multiplayer = is_multiplayer;

  game->player = (GameObject) {
    .bbox = { .position = {0.0, -1.0}, .size = {PLAYER_WIDTH, PLAYER_HEIGHT } },
    .speed = { 0.0, 0.0 },
    .texture = TEXTURE_WHITE,
    .collision_type = COLLISION_PLAYER
  };

  game->opponent = (GameObject) {
    .bbox = { .position = {0.0, 1.0}, .size = {PLAYER_WIDTH, PLAYER_HEIGHT } },
    .speed = { 0.0, 0.0 },
    .texture = TEXTURE_WHITE,
    .collision_type = COLLISION_PLAYER
  };

  game->ball = (GameObject) {
    .bbox = { .position = {0.0, 0.0}, .size = {BALL_WIDTH, BALL_HEIGHT } },
    .speed = { BALL_SPEED, BALL_SPEED },
    .texture = TEXTURE_WHITE,
    .collision_type = COLLISION_BOUNCE
  };

  game->walls[0] = (GameObject) { // Bottom wall
    .bbox = { .position = {-1.0, -1.0 - WALL_THICKNESS}, .size = {WALL_LENGTH, WALL_THICKNESS} },
    .speed = { 0.0, 0.0},
    .texture = TEXTURE_WHITE,
    .collision_type = COLLISION_LOSE
  };

  game->walls[1] = (GameObject) { // Left wall
    .bbox = { .position = {-1.0 - WALL_THICKNESS, -1.0}, .size = {WALL_THICKNESS, WALL_LENGTH } },
    .speed = { 0.0, 0.0},
    .texture = TEXTURE_WHITE,
    .collision_type = COLLISION_BOUNCE
  };

  game->walls[2] = (GameObject) { // Top wall
    .bbox = { .position = {-1.0, 1.0}, .size = {WALL_LENGTH, WALL_THICKNESS} },
    .speed = { 0.0, 0.0},
    .texture = TEXTURE_WHITE,
    .collision_type = is_multiplayer ? COLLISION_WIN : COLLISION_BOUNCE
  };

  game->walls[3] = (GameObject) { // Right wall
    .bbox = { .position = {1.0, -1.0}, .size = {WALL_THICKNESS, WALL_LENGTH } },
    .speed = { 0.0, 0.0},
    .texture = TEXTURE_WHITE,
    .collision_type = COLLISION_BOUNCE
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

void game_advance_time(Game* game, float dt) {
  static const Rectangle board = { .position = { -1.0, -1.0}, .size = { 2.0, 2.0 } };
  GameObject* objects[] = { &game->player, &game->opponent, &game->ball };
  
  for(int i = 0; i < sizeof(objects) / sizeof(GameObject*); ++i) {
    objects[i]->bbox.position = vec2_add(objects[i]->bbox.position, vec2_mul(objects[i]->speed, dt));
    rect_clamp(&objects[i]->bbox, &board);
  }
}

void game_step_end(Game* game, float dt) {
  if (game->state == STATE_LOST || game->state == STATE_WON) {
      // no game logic for this state
      return;
  }

  while (game->state == STATE_RUNNING && dt > 0.0) {
    Collision collision = NO_COLLISION;

    Collision player_collision = find_player_collision(&game->player, &game->ball, dt);
    min_collision(&collision, &player_collision);
    Collision opponent_collision = find_player_collision(&game->opponent, &game->ball, dt);
    min_collision(&collision, &opponent_collision);
    
    for(int i = 0; i < 4; ++i) {
      Collision wall_collision = find_wall_collision(&game->ball, &game->walls[i], dt);
      min_collision(&collision, &wall_collision);
    }

    if(collision.time > dt) {
      game_advance_time(game, dt);
      dt = 0.0;
    }
    else {
      game_advance_time(game, collision.time - EPSILON);
      dt -= collision.time - EPSILON;
      switch(collision.with_what->collision_type) {
        case COLLISION_NONE:
          assert(false);
          break;

        case COLLISION_WIN:
          game->state = STATE_WON;
          break;

        case COLLISION_LOSE:
          game->state = STATE_LOST;
          break;

        case COLLISION_BOUNCE:
          if(collision.normal.x != 0.0) {
            collision.what->speed.x *= -1.0;
          }
          
          if(collision.normal.y != 0.0) {
            collision.what->speed.y *= -1.0;
          }
          break;

        case COLLISION_PLAYER:
          if(collision.with_what->speed.x != 0.0 && (collision.with_what->speed.x > 0.0) != (collision.what->speed.x > 0.0)) {
            collision.what->speed.x *= -1.0;
          }

          collision.what->speed.x *= 1.1;
          collision.what->speed.y *= -1.1;
          break;
      }
    }
  }
}
