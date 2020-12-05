#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

void game_log(LogLevel log_level, const char* file, int line, const char* format, ...) {
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
      level = "INFO ";
      break;
    case LOG_LEVEL_WARNING:
      level = "WARN ";
      break;
    case LOG_LEVEL_ERROR:
      level = "ERROR";
      break;
    case LOG_LEVEL_DEBUG:
      level = "DEBUG";
      break;
  }

  time_t t = time(NULL);
  // NOTE: gmtime uses static buffer, let's hope it is thread-local...
  struct tm utc = *gmtime(&t);
  char log_time[32];
  strftime(log_time, sizeof(log_time), "%T", &utc);


  if(log_level == LOG_LEVEL_DEBUG) {
    fprintf(stderr, "%s [%s] %s:%d %s%s\n", log_time, level, file, line, message, truncated);
  }
  else {
    fprintf(stderr, "%s [%s] %s%s\n", log_time, level, message, truncated);
  }
}
