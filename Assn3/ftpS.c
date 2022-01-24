/** THE UDP SERVER**/
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 

#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <netdb.h>

int main() { 
	int	tcp_sockfd, tcp_newsockfd;

	struct sockaddr_in	tcp_cliaddr, tcp_servaddr;

    // Create tcp socket
	if ((tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}

    // Reset variables
    memset(&tcp_cliaddr, 0, sizeof(tcp_cliaddr));
    memset(&tcp_servaddr, 0, sizeof(tcp_servaddr));
      
    // All this is server info
	tcp_servaddr.sin_family		= AF_INET;
	tcp_servaddr.sin_addr.s_addr	= INADDR_ANY;   
	tcp_servaddr.sin_port		= htons(20000);     // Network Byte Order or big endian system

    // Bind the tcp socket with the server address 
	if (bind(tcp_sockfd, (struct sockaddr *) &tcp_servaddr, sizeof(tcp_servaddr)) < 0) {
		perror("Unable to bind local address\n");
		exit(0);
	}

	listen(tcp_sockfd, 5);  //  5 concurrent connections for tcp

    printf("Server started\n");

	socklen_t tcp_clilen = sizeof(tcp_cliaddr);

    char buf[101], buf2[101]; 
    memset(buf2, '\0', sizeof(buf2));
    memset(buf, '\0', sizeof(buf));

    while(1) {

        // Maintaining file descriptors
        fd_set myfd;
        FD_ZERO(&myfd);
        FD_SET(tcp_sockfd, &myfd);

        // Server waits for a considerably long time for the initial connection
        struct timeval timer;
        timer.tv_sec = 300;
        timer.tv_usec = 0;

        int select_status = select(tcp_sockfd + 1, &myfd, 0, 0, &timer);

        if(select_status < 0) {
            perror("Error in select\n");
            exit(-1);
        }

        // TCP
        if(FD_ISSET(tcp_sockfd, &myfd)) {
            // Handle the tcp connection with a fork (concurrent)
            tcp_newsockfd = accept(tcp_sockfd, (struct sockaddr *) &tcp_cliaddr, &tcp_clilen);
            printf("TCP Connection Requested from %s:%d\n",inet_ntoa(tcp_cliaddr.sin_addr), ntohs(tcp_cliaddr.sin_port));
            if (tcp_newsockfd < 0) {
                perror("Accept error\n");
                exit(0);
            }

            if(fork() == 0) {
                // child: Handles the new tcp connection
                memset(buf, '\0', sizeof(buf));

                int select_status = select(tcp_sockfd + 1, &myfd, 0, 0, 0);

                if(select_status < 0) {
                    perror("Error in select\n");
                    exit(-1);
                }

                int recv_status = recv(tcp_newsockfd, buf, 100, 0);
                if(recv_status < 0) {
                    perror("Error in recv!\n");
                    exit(-1);
                }

                printf("Received: %s\n", buf);

                close(tcp_newsockfd);
                break;
            } else {
                // parent
                continue;
            }
        }

        if(FD_ISSET(tcp_sockfd, &myfd) == 0) {
            // timed out
            printf("Server timed out\n");
            break;
        }
    }

    // Close socket
    close(tcp_sockfd);

    return 0; 
}
