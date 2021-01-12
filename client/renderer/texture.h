#ifndef TEXTURE_H
#define TEXTURE_H

#include "types.h"

typedef struct {
  ObjectID id;
} Texture;

int texture_init(Texture* texture);
void texture_release(Texture* texture);

void texture_bind(Texture* texture);
void texture_unbind(Texture* texture);
void texture_set(Texture* texture, const unsigned char* data, int width, int height);

#endif // TEXTURE_H
