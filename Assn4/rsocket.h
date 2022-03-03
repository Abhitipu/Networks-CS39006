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
// thik hai C chutiya hai
// ruk ruk.. ek karke dekhta pehle
// const int MAX_TABLE_SIZE = 50;
// #define THRESHOLD_TIME 10
#define THRESHOLD_TIME 10  // TODO: maybe change later?
#define MAX_SLEEP 10       // TODO: maybe change later?
#define RECVFROM_SLEEP_TIME 1  // second


// The library functions that are accessible to the user.
int r_socket(int domain, int type, int protocol);

int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

ssize_t r_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);

ssize_t r_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);

int r_close(int fd);

#endif  // _RSOCKET_H