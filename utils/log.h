#ifndef LOG_H
#define LOG_H

typedef enum {
  LOG_LEVEL_INFO,
  LOG_LEVEL_WARNING,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_DEBUG
} LogLevel;

void game_log(LogLevel level, const char* file, int line, const char* format, ...);


#define LOG_INFO(...) game_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...) game_log(LOG_LEVEL_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) game_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#ifdef PONG_DEBUG
#define LOG_DEBUG(...) game_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#else
#define LOG_DEBUG(...) do { } while(0)
#endif // PONG_DEBUG

#endif // LOG_H
