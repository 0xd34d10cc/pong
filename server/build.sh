#! /usr/bin/bash

clang -o connect_server src/*.c -Werror=implicit-function-declaration -Werror=implicit-int
