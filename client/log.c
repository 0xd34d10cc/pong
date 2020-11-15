#include "log.h"

#include <stdarg.h>
#include <stdio.h>

// todo: log time
void game_log(LogLevel level, const char* format, ...) {
  char message[1024];

  va_list args;
  va_start(args, format);
  int n = vsnprintf(message, sizeof(message), format, args);
  va_end(args);

  const char* lvl = "???";
  switch (level) {
    case LOG_LEVEL_INFO:
      lvl = "INFO";
      break;
    case LOG_LEVEL_WARNING:
      lvl = "WARN";
      break;
    case LOG_LEVEL_ERROR:
      lvl = "ERROR";
      break;
  }

  const char* fmt = n <= sizeof(message) ? "[%s] %s\n" : "[%s] %s...\n";
  fprintf(stderr, fmt, lvl, message);
}
