int send_with_ack(int socket, const void *message, size_t length,
                   const struct sockaddr *dest_addr, socklen_t dest_len);

int recv_with_ack(int socket, void *restrict buffer, size_t length,
                  struct sockaddr *fromaddr, socklen_t *fromlen);

int send_file(char *filename, int socket, struct sockaddr *toaddr, socklen_t tolen);

int recv_file(char *filename, int socket, struct sockaddr *fromaddr, socklen_t fromlen);