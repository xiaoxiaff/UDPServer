CC=gcc
CFLAGS=-I.
DEPS = protocol.h
OBJ = protocol.o

all: client serverFork

client.o: client.c $(DEPS)
	$(CC) -c $(CFLAGS) client.c

serverFork.o: serverFork.c $(DEPS)
	$(CC) -c serverFork.c $(CFLAGS)

$(OBJ): protocol.c
	$(CC) -c protocol.c $(CFLAGS)

serverFork: serverFork.o $(OBJ)
	$(CC) -o $@ serverFork.o $(OBJ) $(CFLAGS)

client: client.o $(OBJ)
	$(CC) -o $@ client.o $(OBJ)


