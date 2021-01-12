#include "game.h"
#include "bool.h"

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

  game->player = (Rectangle) { .position = {0.0, -1.0}, .size = {PLAYER_WIDTH, PLAYER_HEIGHT}};
  game->player_speed = (Vec2) {0.0, 0.0};

  game->opponent = (Rectangle) { .position = {0.0, 1.0}, .size = {PLAYER_WIDTH, PLAYER_HEIGHT}};
  game->opponent_speed = (Vec2) {0.0, 0.0};

  game->ball = (Rectangle) {.position = {0.0, 0.0}, .size = {BALL_WIDTH, BALL_HEIGHT}};
  game->ball_speed = (Vec2) {BALL_SPEED, BALL_SPEED};

  game->board = (Rectangle) {.position = {-1.0, -1.0}, .size = {2, 2}};
}

GameState game_state(Game* game) {
  return game->state;
}

void game_event(Game* game, Event event) {
  switch (event) {
    case EVENT_MOVE_LEFT:
      game->player_speed.x = -PLAYER_SPEED;
      break;
    case EVENT_MOVE_RIGHT:
      game->player_speed.x = PLAYER_SPEED;
      break;
    case EVENT_RESTART:
      if (game->state != STATE_RUNNING) {
          game_init(game, game->is_multiplayer);
      }
      break;
  }
}

void game_step_begin(Game* game) {
    game->player_speed.x = 0;
}

void game_set_player_speed(Game* game, Vec2 speed) {
  game->player_speed = speed;
}

void game_update_player_position(Game* game) {
  game->player.position = vec2_add(game->player.position, game->player_speed);
  rect_clamp(&game->player, &game->board);

  game->opponent.position = vec2_add(game->opponent.position, game->opponent_speed);
  rect_clamp(&game->opponent, &game->board);
}

// TODO: Rewrite all rectangles + speed to something like GameObject {Rectangle; Speed}
static void process_player_hit(Game* game, Rectangle* player, Vec2 player_speed) {
  bool curr_intersect = rect_intersect(&game->ball, player);
  Rectangle next_frame_ball = game->ball;
  next_frame_ball.position = vec2_add(next_frame_ball.position, game->ball_speed);
  bool next_intersect = rect_intersect(&next_frame_ball, player);

  if (curr_intersect || next_intersect) {
    game->ball_speed.y = -game->ball_speed.y;
    if (game->ball_speed.y  < 0) {
      game->ball_speed.y -= HIT_SPEED_INC;
    } else {
      game->ball_speed.y += HIT_SPEED_INC;
    }

    if (game->ball_speed.x < 0) {
      game->ball_speed.x -= HIT_SPEED_INC;
    } else {
      game->ball_speed.x += HIT_SPEED_INC;
    }

    if ((player_speed.x != 0) && (player_speed.x > 0) != (game->ball_speed.x > 0)) {
      game->ball_speed.x = -game->ball_speed.x;
    }
  }
}

void game_update_ball_position(Game* game) {
  game->ball.position = vec2_add(game->ball.position, game->ball_speed);
  if (game->ball.position.y < game->board.position.y) {
    game->state = STATE_LOST;
  }

  if (game->ball.position.y > game->board.position.y + game->board.size.y) {
    if (!game->is_multiplayer) {
      game->ball_speed.y = -game->ball_speed.y;
    } 
    else {
      game->state = STATE_WON;
    }
  }

  // floor/wall collisions
  if (game->ball.position.x <= game->board.position.x ||
      game->ball.position.x + game->ball.size.x >= game->board.position.x + game->board.size.x) {
    game->ball_speed.x = -game->ball_speed.x;
  }

  process_player_hit(game, &game->player, game->player_speed);
  process_player_hit(game, &game->opponent, game->opponent_speed);
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
