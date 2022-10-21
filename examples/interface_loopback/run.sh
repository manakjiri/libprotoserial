#!/bin/sh

mkdir -p build &&
echo "# building the program: " &&
cmake -S . -B build &&
cmake --build build &&
echo "# running the program: " &&
./build/libprotoserial_example