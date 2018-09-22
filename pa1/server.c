 #include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <dirent.h>

#include "file_transfer.h"
#include "defs.h"

void do_ls(char *buffer, int sockfd, struct sockaddr *cliaddr, socklen_t clilen) {
    DIR *dir = opendir(".");
    if (dir == NULL) {
        perror("opendir error");
        exit(EXIT_FAILURE);
    }
    FILE *buffer_stream = fmemopen(buffer, MAXLINE, "w");
    if (buffer_stream == NULL) {
        perror("fmemopen error");
        exit(EXIT_FAILURE);
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            fprintf(buffer_stream, "%s\n", entry->d_name);
        }
    }
    fclose(buffer_stream);
    closedir(dir);
    if (send_with_ack(sockfd, buffer, strlen(buffer)+1, cliaddr, clilen) < 0) {
        printf("LS failed\n");
    }
}

void dispatch(char *message, int sockfd, struct sockaddr *cliaddr, socklen_t clilen) {
    char send_buffer[MAXLINE];
    if (!strncmp(message, GET, strlen(GET))) {
        printf("get received\n");
        message += strlen(GET) + 1;
        char* pos;
        if ((pos = strchr(message, '\n')) != NULL) 
            *pos = '\0';

        send_file(message, sockfd, cliaddr, clilen);
    } else if (!strncmp(message, PUT, strlen(PUT))) {
        printf("put received\n");
    } else if (!strncmp(message, DELETE, strlen(DELETE))) {
        printf("delete received\n");
        message += strlen(DELETE) + 1;
        char* pos;
        if ((pos = strchr(message, '\n')) != NULL) 
            *pos = '\0';

        if (remove(message) < 0) {
            sprintf(send_buffer, "Failed to remove file '%s'\n", message);
        } else {
            sprintf(send_buffer, "Successfully removed filed '%s'\n", message);
        }
        send_with_ack(sockfd, send_buffer, strlen(send_buffer)+1, cliaddr, clilen);
    } else if (!strncmp(message, LS, strlen(LS))) {
        printf("ls received\n");
        do_ls(send_buffer, sockfd, cliaddr, clilen);
    } else if (!strncmp(message, EXIT, strlen(EXIT))) {
        printf("exit received\n");
        exit(0);
    } else {
        printf("unknown command received\n");
    }
}

int main() { 
    int sockfd; 
    char recv_buffer[MAXLINE]; //, send_buffer[MAXLINE]; 
    struct sockaddr_in servaddr, cliaddr; 
      
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORT); 
      
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    int n;
    // This has to be initialized or cliaddr wont be filled out in recvfrom
    socklen_t clilen = sizeof cliaddr; 
    while (1) {
        n = recvfrom(sockfd, recv_buffer, sizeof recv_buffer, 0,
                     (struct sockaddr *) &cliaddr, &clilen); 
        if (n < 0) {
            perror("recvfrom error");
        }
        recv_buffer[n] = '\0';

        dispatch(recv_buffer, sockfd, (struct sockaddr *) &cliaddr, clilen);
    }
    
    close(sockfd);
}