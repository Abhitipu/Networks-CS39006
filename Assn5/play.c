#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#define N 52
#define MSG_SIZE 2048
#define MAX_CHAR 100
#define PCKT_LEN 8192
#define TIMEOUT 1
#define MAX_HOP 16
#define DEST_PORT 32164
#define SRC_PORT 8080
#define MAX_TTL 16

#define SEND_MSG 5
#define WAIT_MSG 6
#define END_TRACE 7

int main() {
    printf("%ld %ld\n", sizeof(struct udphdr), sizeof(struct iphdr));
    return 0;
}