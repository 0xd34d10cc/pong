#! /usr/bin/bash

clang -o pong src/*.c -std=c11 -Werror=implicit-function-declaration -Werror=implicit-int -lSDL2
