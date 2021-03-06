#ifndef _RSOCKET_H
#define _RSOCKET_H

#include <stdio.h>
#include <string.h> 
#include <stdlib.h>
#include <unistd.h>  
#include <pthread.h> 
#include <time.h>  
#include <signal.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h> 

#define ERROR -1
#define SOCK_MRP 169        // Our type field in r_socket
#define BUFFER_SIZE 100
#define MAX_TABLE_SIZE 50
#define RECVFROM_SLEEP_TIME 1  // second
#define DROP_PROBABILITY 0.0
#define T 2


// The library functions that are accessible to the user.
int r_socket(int domain, int type, int protocol);

int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

ssize_t r_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);

ssize_t r_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);

int r_close(int fd);

#endif  // _RSOCKET_H