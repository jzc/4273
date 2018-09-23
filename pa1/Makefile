CC = gcc
CFLAGS = -c -g -Wall -Wextra
LFLAGS = -Wall

all: client server

client: client.o file_transfer.o
	mkdir -p client
	$(CC) $(LFLAGS) $^ -o client/$@ 

server: server.o file_transfer.o
	mkdir -p server
	$(CC) $(LFLAGS) $^ -o server/$@

file_transfer.o: file_transfer.c file_transfer.h
	$(CC) $(CFLAGS) $< -o $@

client.o: client.c file_transfer.h defs.h
	$(CC) $(CFLAGS) $< -o $@ 

server.o: server.c file_transfer.h defs.h
	$(CC) $(CFLAGS) $< -o $@

testfiles:
	base64 /dev/urandom | head -c 5K > server/server.txt
	head -c 50M < /dev/urandom > server/server.bin
	base64 /dev/urandom | head -c 5K > client/client.txt
	head -c 50M < /dev/urandom > client/client.bin

clean: 
	rm -rf server client
	rm -f *.o