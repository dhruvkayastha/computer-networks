#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netinet/in.h>	
#include <time.h>
#include <netdb.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <stdbool.h>

#define T 2
#define p 0.50
#define SOCK_MRP 7		//No other SOCK has a value of 7

int r_socket(int domain, int type, int protocol);

int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

ssize_t r_sendto(int sockfd, const void *buf, size_t len, int flags,
                      const struct sockaddr *dest_addr, socklen_t addrlen);

ssize_t r_recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen);

int r_close(int fd);

int dropMessage(float P);
