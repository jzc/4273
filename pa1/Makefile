CC = gcc
CFLAGS = -c -g -Wall -Wextra
LFLAGS = -Wall

all: client server

client: client.o file_transfer.o
	$(CC) $(LFLAGS) $^ -o $@ 

server: server.o file_transfer.o
	$(CC) $(LFLAGS) $^ -o $@

file_transfer.o: file_transfer.c file_transfer.h
	$(CC) $(CFLAGS) $< -o $@

client.o: client.c file_transfer.h defs.h
	$(CC) $(CFLAGS) $< -o $@ 

server.o: server.c file_transfer.h defs.h
	$(CC) $(CFLAGS) $< -o $@

clean: 
	rm -f server client
	rm -f *.o