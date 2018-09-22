#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#include "file_transfer.h"
  
// #define PORT     8080 
// #define MAXLINE 1024
#include "defs.h" 

void dispatch(char *message, int sockfd, struct sockaddr *servaddr, socklen_t servlen) {
    char recv_buffer[MAXLINE];
    if (!strncmp(message, GET, strlen(GET))) {
        message += strlen(GET) + 1;
        char* pos;
        if ((pos = strchr(message, '\n')) != NULL) 
            *pos = '\0';

        recv_file(message, sockfd, servaddr, servlen);
    } else if (!strncmp(message, PUT, strlen(PUT))) {

    } else if (!strncmp(message, DELETE, strlen(DELETE))) {
        if (recv_with_ack(sockfd, recv_buffer, sizeof recv_buffer, servaddr, &servlen) < 0) {
            printf("timed out");
            return;
        }
        printf("%s", recv_buffer);
    } else if (!strncmp(message, LS, strlen(LS))) {
        if (recv_with_ack(sockfd, recv_buffer, sizeof recv_buffer, servaddr, &servlen) < 0) {
            printf("timed out");
            return;
        }
        printf("%s", recv_buffer);
    } else if (!strncmp(message, EXIT, strlen(EXIT))) {
        exit(0);
    } else {
        printf("Invalid command\n");
    }
}

int main(int argc, char *argv[]) { 
    int sockfd; 
    char send_buffer[MAXLINE], recv_buffer[MAXLINE];
    struct sockaddr_in servaddr; 
   
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE);     
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
      
    int n;
    socklen_t servlen = sizeof servaddr;
    while (1) {
        printf("> ");
        fgets(send_buffer, MAXLINE, stdin);

        n = sendto(sockfd, (const char *) send_buffer, strlen(send_buffer)+1, 0,
                   (const struct sockaddr *) &servaddr, servlen); 
        if (n < 0) {
            perror("sendto error");
        }
        
        dispatch(send_buffer, sockfd, (struct sockaddr *) &servaddr, servlen);
    }
  
    close(sockfd); 
    return 0; 
} 