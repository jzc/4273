all: webserver

webserver: webserver.c
	gcc $^ -g -lpthread -o webserver

clean:
	rm -f webserver