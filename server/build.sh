#! /usr/bin/bash

clang -o server scu.c \
      -std=c11 \
      -O2 -flto \
      -fuse-ld=lld \
      -fvisibility=hidden \
      -Werror=implicit-function-declaration \
      -Werror=implicit-int \
      -Werror=int-conversion \
      -Werror=return-type \
      -I../game
