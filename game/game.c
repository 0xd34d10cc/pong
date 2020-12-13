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

void game_init(Game* game, int board_width, int board_height) {
  (void)board_width;
  (void)board_height;

  game->state = STATE_RUNNING;

  game->player = (Rectangle) { .position = {0.0, -1.0}, .size = {PLAYER_WIDTH, PLAYER_HEIGHT}};
  game->player_speed = (Vec2) {0.0, 0.0};

  game->ball = (Rectangle) {.position = {0.0, 0.0}, .size = {BALL_WIDTH, BALL_HEIGHT}};
  game->ball_speed = (Vec2) {BALL_SPEED, BALL_SPEED};

  game->board = (Rectangle) {.position = {-1.0, -1.0}, .size = {2, 2}};
}

GameState game_state(Game* game) {
  return game->state;
}

void game_positions(Game* game, int* player, int* ball_x, int* ball_y) {
  *player = (game->player.position.x + 1) * (DEFAULT_WINDOW_WIDTH / 2);
  *ball_x = (game->ball.position.x + 1) * (DEFAULT_WINDOW_WIDTH / 2);
  *ball_y = DEFAULT_WINDOW_HEIGHT - (game->ball.position.y + game->ball.size.y +1) * (DEFAULT_WINDOW_HEIGHT / 2) ;
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
      if (game->state == STATE_LOST) {
          game_init(game, game->board.size.x, game->board.size.y);
      }
      break;
  }
}

void game_step_begin(Game* game) {
    game->player_speed.x = 0;
}

void game_step_end(Game* game, int ms) {
  // fixme: the code assumes ms == 16
  (void)ms;

  if (game->state == STATE_LOST) {
      // no game logic for this state
      return;
  }

  // update state
  game->player.position = vec2_add(game->player.position, game->player_speed);
  rect_clamp(&game->player, &game->board);


  game->ball.position = vec2_add(game->ball.position, game->ball_speed);
  if (game->ball.position.y < game->board.position.y) {
    game->state = STATE_LOST;
  }

  if (game->ball.position.y >= game->board.position.y + game->board.size.y) {
    game->ball_speed.y = -game->ball_speed.y;
  }

  // floor/wall collisions
  if (game->ball.position.x <= game->board.position.x ||
      game->ball.position.x + game->ball.size.x >= game->board.position.x + game->board.size.x) {
    game->ball_speed.x = -game->ball_speed.x;
  }

  bool curr_intersect = rect_intersect(&game->ball, &game->player);
  Rectangle next_frame_ball = game->ball;
  next_frame_ball.position = vec2_add(next_frame_ball.position, game->ball_speed);
  bool next_intersect = rect_intersect(&next_frame_ball, &game->player);

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

    if ((game->player_speed.x != 0) && (game->player_speed.x > 0) != (game->ball_speed.x > 0)) {
      game->ball_speed.x = -game->ball_speed.x;
    }
  }
}
