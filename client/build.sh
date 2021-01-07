#! /usr/bin/bash

clang -o pong scu.c                         \
      -std=c11                              \
      -O2 -flto                             \
      -fuse-ld=lld                          \
      -fvisibility=hidden                   \
      -Werror=implicit-function-declaration \
      -Werror=implicit-int                  \
      -Werror=int-conversion                \
      -Werror=return-type                   \
      -Werror=unused-variable               \
      -Werror=unused-parameter              \
      -Werror=incompatible-pointer-types    \
      -I..                                  \
      -I../utils                            \
      -D_GNU_SOURCE                         \
      -DPONG_DEBUG                          \
      `pkg-config --cflags --libs sdl2`     \
      `pkg-config --cflags --libs gl`
