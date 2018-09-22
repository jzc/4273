#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h> 

#include "defs.h"

int wait_for_socket(int socket) {
    struct timeval tv = { .tv_sec = TIMEOUT, .tv_usec = 0 };
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
        return n;
    } 
    // acknoledgement dropped
    else {
        printf("timeout reached, assuming ack dropped\n");
        return -1;
    }
}

int recv_with_ack(int socket, void *restrict buffer, size_t length,
                  struct sockaddr *fromaddr, socklen_t *fromlen) {
    int n;
    // struct sockaddr_in fromaddr;
    // socklen_t fromlen = sizeof fromaddr;

    // message received
    if (wait_for_socket(socket)) {
        n = recvfrom(socket, buffer, length, 0, fromaddr, fromlen);
        if (n < 0) {
            perror("recvfrom error");
            exit(EXIT_FAILURE);
        }
        char ack = 'S';
        int m = sendto(socket, &ack, sizeof ack, 0, fromaddr, *fromlen);
        if (m < 0) {
            perror("sendto error");
            exit(EXIT_FAILURE);
        }
        return n;
    }
    // message dropped
    else {
        printf("timeout reached, assuming message dropped\n");
        return -1;
    }
}

struct file_part {
    int i;
    int size;
    int last_part;
    char data[PART_SIZE];
};

int send_file(char *filename, int socket, struct sockaddr *toaddr, socklen_t tolen) {
    printf("Starting file transfer\n");
    FILE *fp = fopen(filename, "r");
    struct file_part current_part;
    int n, i = 0, failed, ret;
    do {
        // Load current part
        n = fread(current_part.data, 1, PART_SIZE, fp);
        current_part.i = i;
        current_part.size = n;
        // Last part will not fill up the whole buffer
        current_part.last_part = (n == PART_SIZE ? 0 : 1);

        // Enter attempt loop, set failed initally to true.
        failed = 1;
        for (int j = 0; j < N_ATTEMPTS; j++) {
            printf("Sending part %d\n", i);
            n = send_with_ack(socket, &current_part, sizeof current_part, toaddr, tolen);
            // n == -1 on no ack, n >= 0 on success. Set failed to false.
            if (n >= 0) {
                printf("Part %d sent\n", i);
                failed = 0;
                break;
            }
            printf("Acknoledgement not received\n");
        }
        if (failed) {
            printf("No response after %d attempts, aborting transfer\n", N_ATTEMPTS);
            ret = 0;
            goto cleanup;
        }
        i += 1;
    } while (!current_part.last_part);
    ret = 1;
    printf("File transfer complete\n");

    cleanup:
    fclose(fp);
    return ret;
}

int recv_file(char *filename, int socket, struct sockaddr *fromaddr, socklen_t fromlen) {
    printf("Starting file reception\n");
    FILE *fp = fopen(filename, "w");
    struct file_part current_part;
    int n, failed;
    do {
        failed = 1;
        for (int j = 0; j < N_ATTEMPTS; j++) {
            printf("Waiting for part\n");
            n = recv_with_ack(socket, &current_part, sizeof current_part, fromaddr, &fromlen);
            if (n >= 0) {
                printf("Part %d received\n", current_part.i);
                failed = 0;
                break;
            }
            printf("Timeout reached, message probably dropped\n");
        }
        if (failed) {
            printf("No reponse after %d attempts, aborting reception\n", N_ATTEMPTS);
            return 0;
        }
        fseek(fp, current_part.i*PART_SIZE, SEEK_SET);
        fwrite(current_part.data, 1, current_part.size, fp);
    } while(!current_part.last_part);

    printf("File reception complete\n");
    fclose(fp);
    return 1;
}