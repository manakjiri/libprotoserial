cmake_minimum_required(VERSION 3.15)
project(libprotoserial_example)

set(CMAKE_CXX_STANDARD 20)

# build the libprotoserial_example executable with the main.cpp file
add_executable(
    libprotoserial_example
    main.cpp
)

include_directories("../../include")
include_directories("../../submodules/etl/include")
