#ifndef LOG_H
#define LOG_H

typedef enum {
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR
} LogLevel;

void game_log(LogLevel level, const char* format, ...);

#define LOG_INFO(format, ...) game_log(LOG_LEVEL_INFO, (format), __VA_ARGS__)
#define LOG_WARN(foramt, ...) game_log(LOG_LEVEL_WARNING, (format), __VA_ARGS__)
#define LOG_ERROR(format, ...) game_log(LOG_LEVEL_ERROR, (format), __VA_ARGS__)

#endif // LOG_H