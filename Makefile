CC=g++
CFLAGS=-I . -std=c++0x -pthread
DEPS = protocol.h
OBJ = protocol.o

all: client serverFork

client.o: client.cpp $(DEPS)
	$(CC) -c client.cpp $(CFLAGS)

serverFork.o: serverFork.cpp $(DEPS)
	$(CC) -c serverFork.cpp $(CFLAGS)

$(OBJ): protocol.c
	$(CC) -c protocol.c $(CFLAGS)

serverFork: serverFork.o $(OBJ)
	$(CC) -o $@ serverFork.o $(OBJ) $(CFLAGS)

client: client.o $(OBJ)
	$(CC) -o $@ client.o $(OBJ) $(CFLAGS)


