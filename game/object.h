#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include "vec2.h"
#include "rectangle.h"
#include "texture_id.h"

typedef struct {
  Rectangle bbox;
  Vec2 speed;
  TextureID texture;
} GameObject;

#endif // GAME_OBJECT_H
