# protoserial library

Library intended for rapid prototyping of embedded devices with support of standard interfaces such as USB, UART and RS485. It supports addressing for interfaces where it makes sense and tries to be as modular and easily extensible as possible.

**The library is in a pre-pre-alpha stage right now with no guarantee of API or architectural stability.**


## TODOs
- unite transmit/receive functions' names
- create interface config class
- loopback interface does not behave as expected when it comes to addressing
- harden upper layers against data corruption (now these mostly assume that the interface will catch all errors)
    - adding a checksum to transfers as well?
- some "endpoint" object that holds all necessary information describing another device on the ports layer
- simplify the observer implementation
    - consider creating observer_single variant where only one slot is available?
- isolate more of the parser functionality from various interface implementations into the parsers namespace
    - even at the cost of overly specific functions
    - this makes these functions testable, we need unit test code coverage for things like uart and usb interfaces 
    - ideally all do_receive functions should be just a collections of parsers:: components without any additional logic


# Development

## Setup for VS Code

c_cpp_properties.json
```
"configurations": [
    {
        "name": "Linux",
        "includePath": [
            "${default}",
            "${workspaceFolder}",
            "${workspaceFolder}/submodules/etl/include",
            "${workspaceFolder}/submodules/jsoncons/include"
        ],
        "defines": [],
        "compilerPath": "/usr/bin/clang",
        "intelliSenseMode": "linux-clang-x64",
        "cppStandard": "c++20"
    }
],
```

## Unit-tests

This project uses [googletest](https://github.com/google/googletest) as the testing framework, google provides an excellent [quick start guide](https://google.github.io/googletest/primer.html) if you are unfamiliar with it. Unit tests are built using cmake. 

Setup and run unit tests by executing `./helpers/run_tests.sh` from the project root directory. More instructions are available at the beginning of the `tests/tests.cpp` file, this file also contains all the test cases.

