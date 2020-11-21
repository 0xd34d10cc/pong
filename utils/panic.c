#include "panic.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void panic(const char* file, int line, const char* format, ...) {
  char message[4096];

  va_list args;
  va_start(args, format);
  vsnprintf(message, sizeof(message), format, args);
  va_end(args);

  fprintf(stderr, "Panic at %s:%d:\n\t%s\n", file, line, message);
  abort();
}
