#ifndef LOG_H
#define LOG_H

typedef enum {
  LOG_LEVEL_INFO,
  LOG_LEVEL_WARNING,
  LOG_LEVEL_ERROR
} LogLevel;

void game_log(LogLevel level, const char* format, ...);

#define VA_COMMA(...) , __VA_ARGS__
#define LOG_INFO(...) game_log(LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_WARN(...) game_log(LOG_LEVEL_WARNING, __VA_ARGS__)
#define LOG_ERROR(...) game_log(LOG_LEVEL_ERROR, __VA_ARGS__)

#endif // LOG_H
