# /usr/bin/bash

clang -o pong src/*.c -Werror=implicit-function-declaration -lSDL2
