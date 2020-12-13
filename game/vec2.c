#include "vec2.h"

Vec2 vec2_add(Vec2 v1, Vec2 v2) {
  return (Vec2) {.x = v1.x + v2.x, .y = v1.y + v2.y};
}


