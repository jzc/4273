#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h> 

int wait_for_socket(int socket) {
    struct timeval tv = { .tv_sec = 2, .tv_usec = 0 };
    fd_set readfds;

    FD_ZERO(&readfds);
    FD_SET(socket, &readfds);
    
    select(socket+1, &readfds, NULL, NULL, &tv);
    return FD_ISSET(socket, &readfds);
}

int send_with_ack(int socket, const void *message, size_t length,
                   const struct sockaddr *dest_addr, socklen_t dest_len) {
    int n;
    n = sendto(socket, message, length, 0, dest_addr, dest_len);
    if (n < 0) {
        perror("sendto error");
        exit(EXIT_FAILURE);
    }

    // acknoledgement recieved
    if (wait_for_socket(socket)) {
        char ack;
        n = recvfrom(socket, &ack, sizeof ack, 0, NULL, NULL);
        if (n < 0) {
            perror("recvfrom error");
            exit(EXIT_FAILURE);
        }
        return 0;
    } 
    // acknoledgement dropped
    else {
        printf("timeout reached, assuming ack dropped\n");
        return -1;
    }
}

int recv_with_ack(int socket, void *restrict buffer, size_t length) {
    int n;
    struct sockaddr_in fromaddr;
    socklen_t fromlen = sizeof fromaddr;

    // message received
    if (wait_for_socket(socket)) {
        n = recvfrom(socket, buffer, length, 0, (struct sockaddr *) &fromaddr, &fromlen);
        if (n < 0) {
            perror("recvfrom error");
            exit(EXIT_FAILURE);
        }
        char ack = 'S';
        sendto(socket, &ack, sizeof ack, 0, (struct sockaddr *) &fromaddr, fromlen);
        return 0;
    }
    // message dropped
    else {
        printf("timeout reached, assuming message dropped\n");
        return -1;
    }

    
}