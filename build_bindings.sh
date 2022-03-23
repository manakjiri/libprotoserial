#!/bin/sh

g++ -Wall -shared -std=c++20 -I. -Isubmodules/pybind11/include -fPIC -I/usr/include/python3.10 -I/home/george/.local/lib/python3.10/site-packages/pybind11/include libprotoserial/python/pybind11.cpp -o protoserial.cpython-310-x86_64-linux-gnu.so
