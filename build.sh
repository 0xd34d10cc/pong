#! /usr/bin/bash

clang -o pong src/scu.c -std=c11 -O2 -flto -fuse-ld=lld -fvisibility=hidden -Werror=implicit-function-declaration -Werror=implicit-int -lSDL2
