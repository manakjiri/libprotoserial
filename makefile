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

#TESTS = $(patsubst $(SDIR)/%.test.cpp,%,$(SRC))
#SRC = $(patsubst %,$(SDIR)/%.cpp,$(_OBJ))

#vpath %.c $(SDIR)/

#all: $(TARGET)

all:
	echo $(TESTS)


%.test: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)


# builds object files
$(OBJ): $(SRC)
	$(CC) -c -o $@ $< $(CFLAGS) 

# builds the final executable from the object files
$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
