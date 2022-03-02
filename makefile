CC = g++

TARGET = test.out
OPT = -pedantic -Wall -std=c++20 -g -pg

SDIR = libprotoserial
TESTDIR = tests
IDIR = .

CFLAGS=-I$(IDIR)


interface:
	$(CC) -o $(TARGET) $(OPT) $(CFLAGS) $(TESTDIR)/interface.cpp

observer:
	$(CC) -o $(TARGET) $(OPT) $(CFLAGS) $(TESTDIR)/observer.cpp

fragmentation:
	$(CC) -o $(TARGET) $(OPT) $(CFLAGS) $(TESTDIR)/fragmentation.cpp
