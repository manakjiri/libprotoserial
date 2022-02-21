CC = g++

TARGET = test.out
OPT = -pedantic -Wall -std=c++20 -g

SDIR = libprotoserial
IDIR = .
ODIR = build
EVALDIR = eval

SRC = $(wildcard $(SDIR)/*.cpp)

LIBS = -lc -lm

CFLAGS=-I$(IDIR)
OBJ = $(patsubst $(SDIR)/%.cpp,$(ODIR)/%.o,$(SRC))

ORDER = \
byte \
container \
stream \

container:
	$(CC) -o $(TARGET) $(OPT) $(CFLAGS) $(SDIR)/container.test.cpp $(SDIR)/container.cpp $(SDIR)/byte.cpp

interface:
	$(CC) -o $(TARGET) $(OPT) $(CFLAGS) $(SDIR)/interface.test.cpp
