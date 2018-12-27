#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>

#define INVALID_USER_PASS "invalid user pass"
#define BUFSIZE 8192

int Socket(int domain, int type, int protocol) {
    int ret;
    if ((ret = socket(domain, type, protocol)) < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }
    return ret;
}

int Bind(int sockfd, struct sockaddr *my_addr, int addrlen) {
    int ret;
    if ((ret = bind(sockfd, my_addr, addrlen)) < 0 ) {
        perror("bind error");
        exit(EXIT_FAILURE);
    }
    return ret;
}

int Listen(int sockfd, int backlog) {
    int ret;
    if ((ret = listen(sockfd, backlog)) < 0) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }
    return ret;
}

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int ret;
    if ((ret = accept(sockfd, addr, addrlen)) < 0) {
        perror("accept error");
        exit(EXIT_FAILURE);
    }
    return ret;
}

int Connect(int sockfd, struct sockaddr *serv_addr, int addrlen) {
    int ret;
    if ((ret = connect(sockfd, serv_addr, addrlen)) < 0) {
        perror("connect error");
        exit(EXIT_FAILURE);
    }
    return ret;
}