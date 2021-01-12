#ifndef PPM_H
#define PPM_H

typedef struct {
  unsigned char* data;
  size_t width;
  size_t height;
} PPM;

int ppm_open(PPM* image, const char* path);
void ppm_close(PPM* image);

#endif // PPM_H
