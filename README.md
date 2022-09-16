# protoserial library

Library intended for rapid prototyping of embedded devices with support of standard interfaces such as USB, UART and RS485. It supports addressing for interfaces where it makes sense and tries to be as modular and easily extensible as possible.

**The library is in a pre-pre-alpha stage right now with no guarantee of API or architectural stability.**


## TODOs
- unite transmit/receive functions' names
- create interface config class
- loopback interface does not behave as expected when it comes to addressing
- harden upper layers against data corruption (now these mostly assume that the interface will catch all errors)
    - adding a checksum to transfers as well?
- some "endpoint" object that holds all necessary information describing another device on the ports later
- simplify the observer implementation
    - get rid of "watch" capability
    - consider creating observer_single variant where only one slot is available?
- isolate more of the parser functionality from various interface implementations into the parsers namespace
    - even at the cost of overly specific functions
    - this makes these functions testable, we need unit test code coverage for things like uart and usb interfaces 
    - ideally all do_receive functions should be just a collections of parsers:: components without any additional logic
