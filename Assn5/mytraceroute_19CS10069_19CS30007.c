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

#define TIMEOUT 1
#define MAX_HOP 16
#define MAX_TTL 16
#define UDP_PAYLOAD 52
#define MSG_LEN 1024
#define DEST_PORT 32164
#define SRC_PORT 8080

#define SEND_MSG 5
#define WAIT_MSG 6
#define END_TRACE 7

// Compute checksum using the standard checksum algorithm
unsigned short csum(unsigned short *buf, int nwords)
{
    unsigned long sum;
    for (sum = 0; nwords > 0; nwords--) {
        sum += *buf++;
        while((sum >> 16) != 0) {
            unsigned short temp = sum&(0xffff);
            unsigned short temp2 = sum >> 16;
            sum = temp + temp2;
        }
    }

    return (unsigned short)(~sum);
}

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        printf("wrong format! Expected mytraceroute <destination domain name>\n");
        exit(EXIT_FAILURE);
    }

    // Checking if the domain name is alright and obtaining ip address
    struct hostent* resp = gethostbyname(argv[1]);
    char dest_ip[32];

    // Error checking
    if(resp != NULL) {
        memset(dest_ip, '\0', sizeof(dest_ip));
        strcpy(dest_ip, inet_ntoa(*((struct in_addr *)resp->h_addr_list[0])));
    } else {
        printf("Invalid domain name!\n");
        exit(EXIT_FAILURE);
    }
    

    int sockfd1 = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
	if(sockfd1 < 0) {
		perror("Error in socket()");
	}
    int sockfd2 = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if(sockfd2 < 0) {
		perror("Error in socket()");
	} 
    
    struct sockaddr_in saddr_raw1, saddr_raw2, dest_addr;

    saddr_raw1.sin_family = AF_INET;
    saddr_raw1.sin_port = htons(SRC_PORT);
    saddr_raw1.sin_addr.s_addr = INADDR_ANY; // inet_addr("127.0.0.1"); //inet_addr(LISTEN_IP);

    saddr_raw2.sin_family = AF_INET;
    saddr_raw2.sin_port = htons(SRC_PORT);
    saddr_raw2.sin_addr.s_addr = INADDR_ANY; // inet_addr("127.0.0.1"); //inet_addr(LISTEN_IP);
    
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DEST_PORT);
    dest_addr.sin_addr.s_addr = inet_addr(dest_ip);

    socklen_t saddr_raw_len = sizeof(struct sockaddr_in);

    if (bind(sockfd1, (struct sockaddr *)&saddr_raw1, saddr_raw_len) < 0)
    {
        perror("raw bind");
        close(sockfd1);
        close(sockfd2);
        exit(1);
    }

    if (bind(sockfd2, (struct sockaddr *)&saddr_raw2, saddr_raw_len) < 0)
    {
        perror("raw bind");
        close(sockfd1);
        close(sockfd2);
        exit(1);
    }

    printf("mytraceroute to %s (%s), %d hops max, %d byte packets\n", argv[1], dest_ip, MAX_HOP, UDP_PAYLOAD);

    int one = 1;
    const int *val = &one;
    // We will include the ip header ourselves
    if (setsockopt(sockfd1, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0)
    {
        fprintf(stderr, "Error: setsockopt. You need to run this program as root\n");
        close(sockfd1);
        close(sockfd2);
        exit(1);
    }

    unsigned char ttl = 1;
    fd_set myfd;
    char payload[UDP_PAYLOAD];
    struct timespec startTime, endTime;
    struct timeval tv, prev_tv;
    int repeats = 0;
    
    int state = SEND_MSG;

    while (state != END_TRACE) {

        if(state == SEND_MSG) {
            char buffer[MSG_LEN];
            struct iphdr *ip = (struct iphdr *)buffer;
            struct udphdr *udp = (struct udphdr *)(buffer + sizeof(struct iphdr));
            
            for(int i = 0; i < UDP_PAYLOAD; i++)
                payload[i] = (char)rand()%256;

            bzero(buffer, sizeof(buffer));

            // IP HEADER
            ip->version = 4; // 0100
            ip->ihl = 5; // 5*4 = 20 bits header length
            ip->tos = 0; // type of serveice: normal
            ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + UDP_PAYLOAD); //https://tools.ietf.org/html/rfc791#page-11
            ip->id = htons(12219);
            ip->ttl = ttl;     // number of hops
            ip->protocol = IPPROTO_UDP; // UDP 17
            // ip->saddr = saddr_raw1.sin_addr.s_addr;
            // ip->daddr = dest_addr.sin_addr.s_addr;
            inet_pton (AF_INET, "0.0.0.0", &(ip->saddr));
            inet_pton (AF_INET, dest_ip, &(ip->daddr));
            ip->check = csum((unsigned short *)buffer, sizeof(struct iphdr) >> 1);

            
            // UDP HEADER
            udp->source = htons(SRC_PORT);
            udp->dest = htons(DEST_PORT);
            udp->len = htons(sizeof(struct udphdr)+UDP_PAYLOAD);
            udp->check = 0;

            fprintf(stderr, "I'm sending the message!\n");
            strcpy(buffer + sizeof(struct iphdr) + sizeof(struct udphdr), payload);
            clock_gettime(CLOCK_REALTIME, &startTime);
            if (sendto(sockfd1, buffer, ntohs(ip->tot_len), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
                perror("sendto()");
                close(sockfd1);
                close(sockfd2);
                exit(1);
            }
            // printf("packet Send %d\n", ttl);

            prev_tv.tv_sec = 1;
            prev_tv.tv_usec = 0;

            state = WAIT_MSG;

        } else if(state == WAIT_MSG) {
            FD_ZERO(&myfd);
            FD_SET(sockfd2, &myfd);

            tv.tv_sec = prev_tv.tv_sec;
            tv.tv_usec = prev_tv.tv_usec;

            int ret = select(sockfd2 + 1, &myfd, 0, 0, &tv);

            if (ret < 0) {
                // Errors -- terminate
                perror("select()\n");
                close(sockfd1);
                close(sockfd2);
                exit(1);
            } else if(ret == 0) {
                // Select timed out
                fprintf(stderr, "Select timed out!\n");
                if(repeats == 3) {
                    // sent 3 times already
                    printf("\n");
                    ttl++;
                    repeats = 0;
                } else {
                    if(repeats == 0)
                        printf("%d", ttl);
                    printf("\t*");
                    repeats++;
                }

                state = SEND_MSG;
            }
            else {
                // Received a message
                fprintf(stderr, "Received a message\n");
                if (FD_ISSET(sockfd2, &myfd)) {
                    fprintf(stderr, "Beep boop! Something in sockfd2\n");

                    char msg[MSG_LEN];
                    int msglen;
                    socklen_t raddr_len = sizeof(saddr_raw1);
                    msglen = recvfrom(sockfd2, msg, MSG_LEN, 0, (struct sockaddr *)&saddr_raw2, &raddr_len);
                    clock_gettime(CLOCK_REALTIME, &endTime);

                    fprintf(stderr, "ICMP RECV FROM %s\n", inet_ntoa(saddr_raw2.sin_addr));

                    // Extract ip header
                    struct iphdr hdrip = *((struct iphdr *)msg);
                    int iphdrlen = sizeof(hdrip);

                    // Extract icmp header
                    struct icmphdr hdricmp = *((struct icmphdr *)(msg + iphdrlen));

                    // read the destination IP
                    struct in_addr saddr_ip;
                    saddr_ip.s_addr = hdrip.saddr;

                    // sanity check
                    if (hdrip.protocol == 1) {
                        fprintf(stderr, "ICMP received\n");
                        if (hdricmp.type == 3) {
                            //  ensure we've reached destination
                            fprintf(stderr, "Destination unreachable\n");
                            if (hdrip.saddr == inet_addr(dest_ip)) {
                                fprintf(stderr, "Yayyy!! Done!!\n");
                                printf("%d\t%s\t%.3f ms\n", ttl, inet_ntoa(saddr_ip), ((endTime.tv_sec- startTime.tv_sec)*((int)1e9) + (endTime.tv_nsec- startTime.tv_nsec))/(1000000.0));
                                close(sockfd1);
                                close(sockfd2);
                                return 0;
                            } else {
                                // Just continue waiting with the remaining time
                                fprintf(stderr, "Not my destination! But how did i reach here!!\n");
                                prev_tv.tv_sec = tv.tv_sec;
                                prev_tv.tv_usec = tv.tv_usec;
                            }

                        }
                        else if (hdricmp.type == 11) {
                            //time exceed
                            fprintf(stderr, "I need more hops!!\n");
                            printf("%d\t%s\t%.3f ms\n", ttl, inet_ntoa(saddr_ip), ((endTime.tv_sec- startTime.tv_sec)*((int)1e9) + (endTime.tv_nsec- startTime.tv_nsec))/(1000000.0));
                            ttl++;
                            repeats = 0;
                            prev_tv.tv_sec = 1;
                            prev_tv.tv_usec = 0;
                            state = SEND_MSG;
                        } else {
                            fprintf(stderr, "Not my packet\n");
                            // drop and wait with remaining time
                            prev_tv.tv_sec = tv.tv_sec;
                            prev_tv.tv_usec = tv.tv_usec;
                        }
                    }
                    else {
                        // Just continue waiting with the remaining time
                        fprintf(stderr, "Not ICMP\n");
                        prev_tv.tv_sec = tv.tv_sec;
                        prev_tv.tv_usec = tv.tv_usec;
                    }
                }
            }
        }
        
        if(ttl >= MAX_TTL)
            state = END_TRACE;
    }

    close(sockfd1);
    close(sockfd2);
    return 0;
}
