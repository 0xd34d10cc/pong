#ifndef VEC2_H
#define VEC2_H

typedef struct Vec2 {
  float x;
  float y;
} Vec2;

static inline Vec2 vec2(float x, float y) {
  return (Vec2){ x, y };
}
Vec2 vec2_add(Vec2 v1, Vec2 v2);
Vec2 vec2_mul(Vec2 v, float x);
#endif //VEC2_H
