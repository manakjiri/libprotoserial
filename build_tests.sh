#!/bin/sh

cmake -S . -B build -DCMAKE_CXX_COMPILER=clang -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS=-pg &&
cmake --build build
