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
  
#define PORT     8080 
#define MAXLINE 1024 

void ls(char *buffer) {
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
}

int main() { 
    int sockfd; 
    char recv_buffer[MAXLINE], send_buffer[MAXLINE]; 
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

        printf("Client : %s", recv_buffer); 
        if (strncmp("get", recv_buffer, 3) == 0) {
            sprintf(send_buffer, "get recieved\n");
        } else if (strncmp("put", recv_buffer, 3) == 0) {
            sprintf(send_buffer, "put recieved\n");
        } else if (strncmp("delete", recv_buffer, 6) == 0) {
            sprintf(send_buffer, "delete recieved\n");
        } else if (strncmp("ls", recv_buffer, 2) == 0) {
            ls(send_buffer);
        } else if (strncmp("exit", recv_buffer, 4) == 0) {
            break;
        } else if (strncmp("test", recv_buffer, 4) == 0) {
            send_with_ack(sockfd, "test string", sizeof "test string", (struct sockaddr *)&cliaddr, clilen);
        } else {
            sprintf(send_buffer, "unknown command\n");
        }

        // printf("sending: %s", send_buffer);
        // n = sendto(sockfd, (const char *)send_buffer, strlen(send_buffer), 0,
        //            (const struct sockaddr *) &cliaddr, clilen); 
        // if (n < 0) {
        //     perror("sendto error");
        // }
        // printf("message sent.\n");  
    }
    
    close(sockfd);
}