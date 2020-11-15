#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

// todo: log time
void game_log(LogLevel log_level, const char* format, ...) {
  char message[1024];

  va_list args;
  va_start(args, format);
  int n = vsnprintf(message, sizeof(message), format, args);
  va_end(args);

  const char* truncated = "";
  if (n > sizeof(message)) {
    truncated = "...";
  }

  const char* level = "???";
  switch (log_level) {
    case LOG_LEVEL_INFO:
      level = "INFO";
      break;
    case LOG_LEVEL_WARNING:
      level = "WARN";
      break;
    case LOG_LEVEL_ERROR:
      level = "ERROR";
      break;
  }

  time_t t = time(NULL);
  // NOTE: localtime uses static buffer, let's hope it is thread-local...
  struct tm calendar_time = *localtime(&t);

  char log_time[32];
  strftime(log_time, sizeof(log_time), "%T", &calendar_time);

  fprintf(stderr, "%s [%s] %s%s\n", log_time, level, message, truncated);
}
