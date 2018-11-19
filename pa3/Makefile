all: webproxy

webproxy: webproxy.cpp
	g++ $^ -std=c++11 -Wall -pthread -g -o webproxy

clean:
	rm -f webproxy