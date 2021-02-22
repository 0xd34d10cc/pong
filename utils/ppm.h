#ifndef PPM_H
#define PPM_H

#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stddef.h>



typedef struct {
  unsigned char* data;
  size_t width;
  size_t height;
} PPM;

int ppm_open(PPM* image, const char* path);
void ppm_close(PPM* image);

#endif // PPM_H
