#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <pthread.h>
#include <signal.h>

void handle_request(int clent);
void sigint_handler(int sig);

int servsocket, clisocket;
int main(int argc, char* argv[]) {
    struct sockaddr_in servaddr;
    signal(SIGINT, sigint_handler);

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 0;
    }
    
    if ((servsocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(atoi(argv[1])); 

    if (bind(servsocket, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ) {
        perror("bind error");
        return 1;
    }

    if (listen(servsocket, 10) < 0) {
        perror("listen error");
        return 1;
    }

    for (;;) {
        if ((clisocket = accept(servsocket, NULL, 0)) < 0) {
            perror("accept error");
            return 1;
        }
        pthread_t thread;
        if (pthread_create(&thread, NULL, (void * (*)(void *))handle_request, (void *)clisocket)) {
            perror("Sad");
        } 
    }
}

#define BUFSIZE 100000

void handle_request(int client) {
    char *recvbuf = malloc(BUFSIZE), *sendbuf = malloc(BUFSIZE);
    char *token, *curr=recvbuf;
    recv(client, recvbuf, BUFSIZE, 0);
    if ((token = strsep(&curr, " ")) == NULL) {
        return;
    }
    if (!strcmp(token, "GET")) {
        // handle get
        token = strsep(&curr, " ");
        if (!strcmp(token, "/")) {
            token = "/index.html";
        }
        char filepath[3+strlen(token)+1];
        strcpy(filepath, "www");
        strcat(filepath, token);

        FILE *f = fopen(filepath, "rb");
        if (f) {
            // File found; 200
            char *header = "HTTP/1.1 200 OK\r\n\r\n";
            strsep(&curr, "."); 

            send(client, header, strlen(header), 0);

            fseek(f, 0, SEEK_END);
            int size = ftell(f);
            fseek(f, 0, SEEK_SET);

            void *fmap = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fileno(f), 0);
            send(client, fmap, size, 0);
        } else {
            // File not found; 404
        }
        close(client);


    } else if (!strcmp(token, "POST")) {
        // handle post
    } else {
        // handle other methods
    }

    free(recvbuf);
    free(sendbuf);
    return;
}

void sigint_handler(int sig) {
    printf("Shutting down server\n");
    close(servsocket);
    close(clisocket);
    exit(0);
}