#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <errno.h>

#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <netdb.h>
#include<netinet/udp.h>	//Provides declarations for udp header
#include<netinet/ip.h>	//Provides declarations for ip header

#define BUFFER_LEN 100
#define DEST_PORT 32164
#define SRC_PORT 20000

/*
	Generic checksum calculation function
*/

struct pseudo_header
{
	u_int32_t source_address;
	u_int32_t dest_address;
	u_int8_t placeholder;
	u_int8_t protocol;
	u_int16_t udp_length;
};

unsigned short csum(unsigned short *ptr,int nbytes) 
{
	register long sum;
	unsigned short oddbyte;
	register short answer;

	sum=0;
	while(nbytes>1) {
		sum+=*ptr++;
		nbytes-=2;
	}
	if(nbytes==1) {
		oddbyte=0;
		*((u_char*)&oddbyte)=*(u_char*)ptr;
		sum+=oddbyte;
	}

	sum = (sum>>16)+(sum & 0xffff);
	sum = sum + (sum>>16);
	answer=(short)~sum;
	
	return(answer);
}

int main(int argc, char *argv[]) {
    if(argc != 2)
    {
        printf("wrong format! Expected mytraceroute <destination domain name>\n");
        exit(EXIT_FAILURE);
    }

    struct hostent* resp = gethostbyname(argv[1]);
    char dest_ip[32];
    // Error checking
    if(resp != NULL) {
        int tot_ips= 0;
        // currently taking the last ip
        for(int i = 0; resp->h_addr_list[tot_ips] != NULL; i++, tot_ips++) {
            memset(dest_ip, '\0', sizeof(dest_ip));
            strcpy(dest_ip, inet_ntoa(*((struct in_addr *)resp->h_addr_list[i])));
        }
    } else {
        printf("Invalid domain name!\n");
        exit(EXIT_FAILURE);
    }

    int sockfd1 = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if(sockfd1 < 0) {
		perror("Error in socket()");
	}
    int sockfd2 = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if(sockfd2 < 0) {
		perror("Error in socket()");
	}

    int one = 1;
    const int *val = &one;

    if (setsockopt (sockfd1, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0)
    {
        printf ("Error setting IP_HDRINCL. Error number : %d . Error message : %s \n" , errno , strerror(errno));
        exit(0);
    }

	//Datagram to represent the packet
	char datagram[4096] , source_ip[32] , *data , *pseudogram;
	
	//zero out the packet buffer
	memset(datagram, 0, 4096);
	
	//IP header
	struct iphdr *iph = (struct iphdr *) datagram;
	
	//UDP header
	struct udphdr *udph = (struct udphdr *) (datagram + sizeof (struct iphdr));
	
	struct sockaddr_in dest;
	struct pseudo_header psh;
	
	//Data part
	data = datagram + sizeof(struct iphdr) + sizeof(struct udphdr);
	strcpy(data, "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ");
	
	//some address resolution
	strcpy(source_ip , "127.0.0.1");
	
	dest.sin_family = AF_INET;
	dest.sin_port = htons(DEST_PORT);
	dest.sin_addr.s_addr = inet_addr(dest_ip);
	
	//Fill in the IP Header
	iph->ihl = 5;
	iph->version = 4;
	iph->tos = 0;
	iph->tot_len = sizeof (struct iphdr) + sizeof (struct udphdr) + strlen(data);
	iph->id = htonl (54321);	//Id of this packet
	iph->frag_off = 0;
	iph->ttl = 255; // ttl
	iph->protocol = IPPROTO_UDP;
	iph->check = 0;		//Set to 0 before calculating checksum
	iph->saddr = inet_addr (source_ip);	//Spoof the source ip address
	iph->daddr = dest.sin_addr.s_addr;
	
	//Ip checksum
	iph->check = csum ((unsigned short *) datagram, iph->tot_len);
    /* UDP header as specified by RFC 768, August 1980. 
#ifdef __FAVOR_BSD

    struct udphdr {
        u_int16_t uh_sport;  /* source port 
        u_int16_t uh_dport;  /* destination port 
        u_int16_t uh_ulen;   /* udp length 
        u_int16_t uh_sum;    /* udp checksum 
    };

#else

    struct udphdr {
        u_int16_t source;
        u_int16_t dest;
        u_int16_t len;
        u_int16_t check;
    };

#endif
    */

	//UDP header
	udph->source = htons (SRC_PORT);
	udph->dest = htons (DEST_PORT);
	udph->len = htons(8 + strlen(data));	//udp header size
	udph->check = 0;	//leave checksum 0 now, filled later by pseudo header
	
	//Now the UDP checksum using the pseudo header
	psh.source_address = inet_addr(source_ip);
	psh.dest_address = dest.sin_addr.s_addr;
	psh.placeholder = 0;
	psh.protocol = IPPROTO_UDP;
	psh.udp_length = htons(sizeof(struct udphdr) + strlen(data));
	
	int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + strlen(data);
	pseudogram = malloc(psize);
	
	memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
	memcpy(pseudogram + sizeof(struct pseudo_header) , udph , sizeof(struct udphdr) + strlen(data));
	
	udph->check = csum((unsigned short*) pseudogram , psize);
	
	//loop if you want to flood :)
	//while (1)
	{
		//Send the packet
		if (sendto (sockfd1, datagram, iph->tot_len , 0, (struct sockaddr *) &dest, sizeof (dest)) < 0)
		{
			perror("sendto failed");
		}
		//Data send successfully
		else
		{
			printf ("Packet Send. Length : %d \n" , iph->tot_len);
		}
	}
    int ttl = 1;

    return 0;
}