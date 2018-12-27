CFLAGS=-std=c++11 -Wall -pthread -g 

all: dfc dfs

dfc: dfc.cpp helper.h
	g++ $^ $(CFLAGS) -o $@

dfs: dfs.cpp helper.h
	g++ $^ $(CFLAGS) -o $@

clean:
	rm -f dfc dfs