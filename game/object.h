#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include "vec2.h"
#include "rectangle.h"
#include "texture_id.h"

typedef enum {
  COLLISION_NONE,
  COLLISION_WIN,
  COLLISION_LOSE,
  COLLISION_BOUNCE,
  COLLISION_PLAYER
} CollisionType;

typedef struct {
  Rectangle bbox;
  Vec2 speed;
  TextureID texture;
  CollisionType collision_type;
} GameObject;

#endif // GAME_OBJECT_H
