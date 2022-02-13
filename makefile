CC = g++

TARGET = test.out
OPT = -pedantic -Wall -std=c++17 -g

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
	$(CC) -o $(TARGET) $(OPT) -I$(IDIR) $(SDIR)/container.test.cpp $(SDIR)/container.cpp $(SDIR)/byte.cpp


