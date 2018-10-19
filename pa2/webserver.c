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

void sigint_handler(int sig);
void *handle_request(void *clip);
char *get_contenttype(char *filepath);

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
        int *clip = malloc(sizeof clisocket);
        *clip = clisocket;
        if (pthread_create(&thread, NULL, handle_request, clip)) {
            perror("pthread_create error");
        } 
        // handle_request(clip);
    }
}

void sigint_handler(int sig) {
    printf("Shutting down server\n");
    close(servsocket);
    close(clisocket);
    exit(0);
}

#define BUFSIZE 2048

void *handle_request(void *clip) {
    int client = * (int *)clip;
    char recvbuf[BUFSIZE], headerbuf[BUFSIZE], filepath[BUFSIZE];
    char *token, *curr=recvbuf;
    free(clip);

    recv(client, recvbuf, BUFSIZE, 0);
    if ((token = strsep(&curr, " ")) == NULL) {
        return NULL;
    }
    if (!strcmp(token, "GET")) {
        // handle get
        token = strsep(&curr, " ");
        printf("GET %s\n", token);
        if (!strcmp(token, "/")) {
            token = "/index.html";
        }
        sprintf(filepath, "www%s", token);

        FILE *f = fopen(filepath, "rb");
        if (f) {
            // File found; 200

            // Get size of file
            fseek(f, 0, SEEK_END);
            int size = ftell(f);
            fseek(f, 0, SEEK_SET);

            // Get content type
            char *contenttype = get_contenttype(filepath);            

            FILE *header = fmemopen(headerbuf, 1024, "w");
            fprintf(header, "HTTP/1.1 200 OK\r\n");
            fprintf(header, "Server: PA2 4273\r\n");
            fprintf(header, "Content-Type: %s\r\n", contenttype);
            fprintf(header, "Content-Length: %d\r\n", size);
            fprintf(header, "\r\n");
            fclose(header);
            send(client, headerbuf, strlen(headerbuf), 0);

            // Memory map file and send the rest of the response
            void *fmap = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fileno(f), 0);
            send(client, fmap, size, 0);
            munmap(fmap, size);
            fclose(f);
        } else {
            // File not found; 404
        }
    } else if (!strcmp(token, "POST")) {
        // handle post
    } else {
        // handle other methods
    }

    close(client);
    return NULL;
}

char *get_contenttype(char *filepath) {
    char *p;
    for (p = filepath; *p != '\0'; ++p);
    for (; (*p != '.') && (p != filepath); --p);
    if (!strcmp(p, ".html")) {
        return "text/html";
    }
    if (!strcmp(p, ".txt")) {
        return "text/plain";
    }
    if (!strcmp(p, ".png")) {
        return "image/png";
    }
    if (!strcmp(p, ".gif")) {
        return "image/gif";
    }
    if (!strcmp(p, ".png")) {
        return "image/png";
    }
    if (!strcmp(p, ".css")) {
        return "text/css";
    }
    if (!strcmp(p, ".js")) {
        return "application/javascript";
    }
    return "text/plain";
}