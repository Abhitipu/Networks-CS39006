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

/* Function to generate random string */
void gen(char *dst)
{
    for (int i = 0; i < N; i++)
    {
        dst[i] = rand() % 26 + 'A';
    }
    dst[N-1] = '\0';
}

/*The IPv4 layer generates an IP header when sending a packet unless the IP_HDRINCL socket option is enabled on the socket. 
When it is enabled, the packet must contain an IP header.*/

/* Function to find IP  */
int hostname_to_ip(char *hostname, char *ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    if ((he = gethostbyname(hostname)) == NULL)
    {
        // get the host info
        herror("gethostbyname");
        return 1;
    }

    addr_list = (struct in_addr **)he->h_addr_list;
    if (addr_list[0] == NULL)
        return 1;
    else
    {
        strcpy(ip, inet_ntoa(*addr_list[0]));
        return 0;
    }
}
/* Check sum function  */
unsigned short csum(unsigned short *buf, int nwords)
{
    unsigned long sum;
    for (sum = 0; nwords > 0; nwords--)
        sum += *buf++;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
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
        // currently taking the first ip
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

    struct sockaddr_in saddr_raw, cli_addr;

    saddr_raw.sin_family = AF_INET;
    saddr_raw.sin_port = htons(SRC_PORT);
    saddr_raw.sin_addr.s_addr = INADDR_ANY; //inet_addr(LISTEN_IP);
    socklen_t saddr_raw_len = sizeof(saddr_raw);

    /* 3. Bind the Sockets */
    if (bind(sockfd1, (struct sockaddr *)&saddr_raw, saddr_raw_len) < 0)
    {
        perror("raw bind");
        close(sockfd1);
        close(sockfd2);
        exit(1);
    }

    printf("mytraceroute to %s (%s), %d hops max, %d byte packets\n", argv[1], dest_ip, MAX_HOP, N);

    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(DEST_PORT);
    cli_addr.sin_addr.s_addr = inet_addr(dest_ip);

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

    int ttl = 1;
    fd_set myfd;
    char payload[52];
    clock_t start_time;
    struct timeval tv, prev_tv;
    int repeats = 0;
    int timed_out = 0;
    int state = SEND_MSG;

    while (state != END_TRACE) {

        if(state == SEND_MSG) {
            char buffer[PCKT_LEN];
            struct iphdr *ip = (struct iphdr *)buffer;
            struct udphdr *udp = (struct udphdr *)(buffer + sizeof(struct iphdr));

            /* 4. generate Payload */
            gen(payload);
            bzero(buffer, sizeof(buffer));

            /* 5. Generate UPD and IP header */
            ip->ihl = 5;
            ip->version = 4;
            ip->tos = 0; // low delay
            ip->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + N; //https://tools.ietf.org/html/rfc791#page-11
            ip->id = htons(12219);
            ip->ttl = ttl;     // hops
            ip->protocol = 17; // UDP
            ip->saddr = 0;     //src_addr;
            ip->daddr = inet_addr(dest_ip);

            // fabricate the UDP header
            udp->source = htons(SRC_PORT);
            // destination port number
            udp->dest = htons(DEST_PORT);
            udp->len = htons(sizeof(struct udphdr)+N);

            // calculate the checksum for integrity
            ip->check = csum((unsigned short *)buffer, sizeof(struct iphdr) + sizeof(struct udphdr));

            /* 6. Send the packet */
            fprintf(stderr, "I'm sending the message!\n");
            strcpy(buffer + sizeof(struct iphdr) + sizeof(struct udphdr), payload);
            if (sendto(sockfd1, buffer, ip->tot_len, 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr)) < 0) {
                perror("sendto()");
                close(sockfd1);
                close(sockfd2);
                exit(1);
            }
            // printf("packet Send %d\n", ttl);
            start_time = clock();

            prev_tv.tv_sec = 1;
            prev_tv.tv_usec = 0;

            state = WAIT_MSG;

        } else if(state == WAIT_MSG) {
            /* 7. Wait on select call */
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
            } else if(tv.tv_sec == 0 && tv.tv_usec == 0) {
                // Select timed out
                fprintf(stderr, "Select timed out!\n");
                if(repeats == 3) {
                    // sent 3 times already
                    printf("%d\t*\t*\t*\t*\t\n",ttl);
                    ttl++;
                    repeats = 0;
                } else
                    repeats++;

                state = SEND_MSG;
            }
            else {
                // Received a message
                fprintf(stderr, "Received a message\n");
                if (FD_ISSET(sockfd2, &myfd)) {
                    /* 8. Read the ICMP Message */
                    fprintf(stderr, "Beep boop! Something in sockfd2\n");
                    char msg[MSG_SIZE];
                    int msglen;
                    socklen_t raddr_len = sizeof(saddr_raw);
                    msglen = recvfrom(sockfd2, msg, MSG_SIZE, 0, (struct sockaddr *)&saddr_raw, &raddr_len);
                    clock_t end_time = clock();

                    // Continue with select {empty packet}
                    if (msglen <= 0) {
                        fprintf(stderr, "BLANK :(\n");
                        prev_tv.tv_sec = tv.tv_sec;
                        prev_tv.tv_usec = tv.tv_usec;
                        continue;
                    }

                    // Extract ip header
                    struct iphdr hdrip = *((struct iphdr *)msg);
                    int iphdrlen = sizeof(hdrip);

                    // Extract icmp header
                    struct icmphdr hdricmp = *((struct icmphdr *)(msg + iphdrlen));

                    /* 9. Handle Different Case */
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
                                printf("%d\t%s\t%.3f ms\n", ttl, inet_ntoa(saddr_ip), (float)(end_time - start_time) / CLOCKS_PER_SEC * 1000);
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
                            printf("%d\t%s\t%.3f ms\n", ttl, inet_ntoa(saddr_ip), (float)(end_time - start_time) / CLOCKS_PER_SEC * 1000);
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
