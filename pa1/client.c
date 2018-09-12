#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#include "file_transfer.h"
  
#define PORT     8080 
#define MAXLINE 1024 
  
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
        n = sendto(sockfd, (const char *) send_buffer, strlen(send_buffer), 0,
                   (const struct sockaddr *) &servaddr, servlen); 
        if (n < 0) {
            perror("sendto error");
        }
        
        printf("%d\n", recv_with_ack(sockfd, recv_buffer, MAXLINE));

        // recvfrom()
        // n = recvfrom(sockfd, recv_buffer, MAXLINE, 0, (
            
        // n = recvfrom(sockfd, (char *)recv_buffer, MAXLINE, 0,
        //              (struct sockaddr *) &servaddr, &servlen); 
        // if (n < 0) {
        //     perror("recvfrom error");
        // }
        // printf("Server: %s", recv_buffer); 
    }
  
    close(sockfd); 
    return 0; 
} 