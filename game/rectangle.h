#ifndef RECTANGLE_H
#define RECTANGLE_H

#include "vec2.h"

typedef struct Rectangle {
  Vec2 position; // left bottom corner
  Vec2 size;
} Rectangle;

inline static bool in_range(float val, float min, float max) {
  return (val >= min) && (val <= max);
}

inline static bool rect_intersect(const Rectangle* r1, const Rectangle* r2) {
  float r1_top_right_x = r1->position.x + r1->size.x;
  float r1_top_right_y = r1->position.y + r1->size.y;


  float r2_top_right_x = r2->position.x + r2->size.x;
  float r2_top_right_y = r2->position.y + r2->size.y;


  // x intersect()

  bool x_overlap = in_range(r1->position.x, r2->position.x, r2_top_right_x)
    || in_range(r2->position.x, r1->position.x, r1_top_right_x);
  bool y_overlap = in_range(r1->position.y, r2->position.y, r2_top_right_y)
    || in_range(r2->position.y, r1->position.y, r1_top_right_y);

  return x_overlap && y_overlap;
}

inline static void rect_clamp(Rectangle* inner, Rectangle* outer) {
  if (inner->position.x + inner->size.x > outer->position.x + outer->size.x) {
    inner->position.x = (outer->position.x + outer->size.x) - inner->size.x;
  }

  if (inner->position.x < outer->position.x) {
    inner->position.x = outer->position.x;
  }

  if (inner->position.y < outer->position.y) {
    inner->position.y = outer->position.y;
  }

  if (inner->position.y + inner->size.y > outer->position.y + outer->size.y) {
    inner->position.y = (outer->position.y + outer->size.y) - inner->size.y;
  }

}


#endif // RECTANGLE_H
