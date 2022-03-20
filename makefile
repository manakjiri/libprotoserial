CC = g++

TARGET = test.out
OPT = -pedantic -Wall -std=c++20 -g -pg

SDIR = libprotoserial
TESTDIR = tests
IDIRS = . submodules/serial/include/

CFLAGS=$(patsubst %, -I%, $(IDIRS))


interface:
	$(CC) -o $(TARGET) $(OPT) $(CFLAGS) $(TESTDIR)/interface.cpp

observer:
	$(CC) -o $(TARGET) $(OPT) $(CFLAGS) $(TESTDIR)/observer.cpp

fragmentation:
	$(CC) -o $(TARGET) $(OPT) $(CFLAGS) $(TESTDIR)/fragmentation.cpp

linux_uart:
	$(CC) -o $(TARGET) $(OPT) $(CFLAGS) $(TESTDIR)/linux_uart.cpp
