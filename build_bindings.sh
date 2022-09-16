#!/bin/sh

#OUT="protoserial.cpython-310-x86_64-linux-gnu.so"
OUT="protoserial$(python3-config --extension-suffix)"
#$(python3 -m pybind11 --includes)
#-I~/.local/lib/python3.10/site-packages/pybind11/include

g++ -Wall -shared -std=c++20 -fPIC -I. -Isubmodules/pybind11/include -Isubmodules/etl/include -I/usr/include/python3.10 $(python3 -m pybind11 --includes) python/pybind11.cpp -o python/$OUT
