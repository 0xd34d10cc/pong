#include "ppm.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

void ppm_close(PPM* image) {
  free(image->data);
}

// P3
// 4 4
// 15
// 0  0  0    0  0  0    0  0  0   15  0 15
// 0  0  0    0 15  7    0  0  0    0  0  0
// 0  0  0    0  0  0    0 15  7    0  0  0
// 15  0 15    0  0  0    0  0  0    0  0  0
static int ppm_load(PPM* image, FILE* file) {
  char buffer[128];

  if (!fgets(buffer, sizeof(buffer), file)) {
    return -1;
  }

  bool plain = false;

  if (strcmp(buffer, "P3\n") == 0) {
    plain = true;;
  } else if (strcmp(buffer, "P6\n") == 0) {
    plain = false;
  } else {
    LOG_DEBUG("Invalid ppm magic number: %s", buffer);
    return -1;
  }

  if (fscanf(file, "%zu %zu", &image->width, &image->height) != 2) {
    return -1;
  }

  size_t max_pixel_value;
  if (fscanf(file, "%zu ", &max_pixel_value) != 1 || max_pixel_value != 255) {
    return -1;
  }

  size_t n = image->width * image->height;
  image->data = malloc(n * 3);

  if (plain) {
    for (int i = 0; i < n; ++i) {
      int r, g, b;
      if (fscanf(file, "%d %d %d", &r, &g, &b) != 3) {
        free(image->data);
        return -1;
      }

      image->data[i * 3 + 0] = (unsigned char)r;
      image->data[i * 3 + 1] = (unsigned char)g;
      image->data[i * 3 + 2] = (unsigned char)b;
    }
  } else {
    if (fread(image->data, 3, n, file) != n) {
      free(image->data);
      return -1;
    }
  }

  return 0;
}

int ppm_open(PPM* image, const char* path) {
  FILE* f = fopen(path, "rb");
  if (f == NULL) {
    LOG_DEBUG("Failed to open image at %s: %s", path, strerror(errno));
    return -1;
  }

  if (ppm_load(image, f) < 0) {
    return -1;
  }

  fclose(f);
  return 0;
}

