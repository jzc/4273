
// Client side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
  
#define PORT     8080 
#define MAXLINE 1024 
  
// Driver code 
int main(int argc, char *argv[]) { 
    int sockfd; 
    char send_buffer[MAXLINE], recv_buffer[MAXLINE];
    struct sockaddr_in servaddr; 
  
    // Creating socket file descriptor 
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 

    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
      
    unsigned int n, len; 
    
    while (1) {
        fgets(send_buffer, MAXLINE, stdin);
        sendto(sockfd, (const char *) send_buffer, strlen(send_buffer), 
            MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
                sizeof(servaddr)); 
        // printf("\"%s\" sent.\n", send_buffer);
            
        n = recvfrom(sockfd, (char *)recv_buffer, MAXLINE,  
                    MSG_WAITALL, (struct sockaddr *) &servaddr, 
                    &len); 
        recv_buffer[n] = '\0'; 
        printf("Server : %s", recv_buffer); 
    }
  
    close(sockfd); 
    return 0; 
} 