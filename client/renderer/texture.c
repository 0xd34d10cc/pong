#include "texture.h"

#ifdef WIN32
#include <windows.h>
#include <gl/GL.h>
#endif

int texture_init(Texture* texture) {
  glGenTextures(1, &texture->id);
  texture_bind(texture);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  return 0;
}

void texture_release(Texture* texture) {
  glDeleteTextures(1, &texture->id);
}

void texture_bind(Texture* texture) {
  glBindTexture(GL_TEXTURE_2D, texture->id);
}

void texture_set(Texture* texture, const unsigned char* data, int width, int height) {
  (void)texture;
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
}

void texture_unbind(Texture* texture) {
  (void)texture;
  glBindTexture(GL_TEXTURE_2D, 0);
}
