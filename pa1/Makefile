CC = gcc
CFLAGS = -g -Wall

all: client server

client: client.c
	$(CC) $(CFLAGS) $^ -o $@ 

server: server.c
	$(CC) $(CFLAGS) $^ -o $@

clean: 
	rm -f server client