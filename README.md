# protoserial library

Library intended for rapid prototyping of embedded devices with support of standard interfaces such as USB, UART and RS485. It supports addressing for interfaces where it makes sense and tries to be as modular and easily extensible as possible.

**The library is in a pre-pre-alpha stage right now with no guarantee of API or architectural stability.**


## TODOs
- unite transmit/receive functions' names
- create interface config class
- loopback interface does not behave as expected when it comes to addressing
- harden upper layers against data corruption (now these mostly assume that the interface will catch all errors)
    - adding a checksum to transfers as well?
