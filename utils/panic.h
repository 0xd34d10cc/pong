#ifndef PANIC_H
#define PANIC_H

void panic(const char* file, int line, const char* format, ...);

#define PANIC(...) panic(__FILE__, __LINE__, __VA_ARGS__)

#endif // PANIC_H
