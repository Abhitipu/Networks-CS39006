// DNS server to handle both tcp and ip
// Get a domain name from the client
// Return corresponding ip address using the gethostbyname method

// CONCURRENT SERVER : We're gonna fork here : P
// Also we'll need to open 2 sockets and handle both of them!!

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

int Max(int a, int b) {
    return (a > b) ? a : b;
}

int main() { 
    int udp_sockfd; 
	int	tcp_sockfd, tcp_newsockfd;

    struct sockaddr_in udp_servaddr, udp_cliaddr; 
	struct sockaddr_in	tcp_cliaddr, tcp_servaddr;

    // Create udp socket
    udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sockfd < 0) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    // Create tcp socket
	if ((tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}

    // Reset variables
    memset(&udp_servaddr, 0, sizeof(udp_servaddr)); 
    memset(&udp_cliaddr, 0, sizeof(udp_cliaddr)); 

    memset(&tcp_cliaddr, 0, sizeof(tcp_cliaddr));
    memset(&tcp_servaddr, 0, sizeof(tcp_servaddr));
      
    // Server specifications
    udp_servaddr.sin_family    = AF_INET; 
    udp_servaddr.sin_addr.s_addr = INADDR_ANY; 
    udp_servaddr.sin_port = htons(8181); 
      
    // All this is server info
	tcp_servaddr.sin_family		= AF_INET;
	tcp_servaddr.sin_addr.s_addr	= INADDR_ANY;   
	tcp_servaddr.sin_port		= htons(20000);     // Network Byte Order or big endian system

    // Bind the socket with the server address 
    if(bind(udp_sockfd, (const struct sockaddr *)&udp_servaddr, sizeof(udp_servaddr)) < 0 ) { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 

    //  We will bind it to a port on our machine
	if (bind(tcp_sockfd, (struct sockaddr *) &tcp_servaddr, sizeof(tcp_servaddr)) < 0) {
        // There is a global variable called errno which gets set in case of an error
		perror("Unable to bind local address\n");
		exit(0);
	}

	listen(tcp_sockfd, 5);  //  5 concurrent connections

    printf("Server started\n");

    fd_set myfd;
    FD_ZERO(&myfd);
    FD_SET(udp_sockfd, &myfd);
    FD_SET(tcp_sockfd, &myfd);

    socklen_t udp_clilen = sizeof(udp_cliaddr);
	socklen_t tcp_clilen = sizeof(tcp_cliaddr);

    char buf[101], buf2[101]; 
    memset(buf2, '\0', sizeof(buf2));
    memset(buf, '\0', sizeof(buf));


    while(1) {
        struct timeval timer;
        timer.tv_sec = 100;
        timer.tv_usec = 0;

        int select_status = select(Max(udp_sockfd, tcp_sockfd) + 1, &myfd, 0, 0, &timer);

        if(select_status < 0) {
            perror("Error in select\n");
            exit(-1);
        }

        if(FD_ISSET(tcp_sockfd, &myfd)) {
            // Handle the tcp connection with a fork (concurrent)
            tcp_newsockfd = accept(tcp_sockfd, (struct sockaddr *) &tcp_cliaddr, &tcp_clilen);

            if (tcp_newsockfd < 0) {
                perror("Accept error\n");
                exit(0);
            }

            if(fork() == 0) {
                // child: Need to figure this out
                memset(buf, '\0', sizeof(buf));

                timer.tv_sec = 2;
                timer.tv_usec = 0;

                int select_status = select(Max(udp_sockfd, tcp_sockfd) + 1, &myfd, 0, 0, &timer);

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

                struct hostent* resp = gethostbyname(buf);
                if(resp != NULL) {
                    int tot_ips= 0;
                    for(; resp->h_addr_list[tot_ips] != NULL; tot_ips++);

                    int send_status = send(tcp_newsockfd, &tot_ips, sizeof(int), 0);
                    if(send_status < 0) {
                        perror("Send failed!\n");
                        exit(-1);
                    }

                    for(int i = 0; i < tot_ips; i++) {
                        memset(buf, '\0', sizeof(buf2));
                        strcpy(buf, inet_ntoa(*((struct in_addr *)resp->h_addr_list[i])));
                        int send_status = send(tcp_newsockfd, buf, 100, 0);
                        if(send_status < 0) {
                            perror("Send failed from server\n");
                            exit(-1);
                        }
                    }

                } else {
                    // Send -1 for error
                    int tot_ips = -1;
                    int send_status = send(tcp_newsockfd, &tot_ips, sizeof(int), 0);
                    if(send_status < 0) {
                        perror("Send failed!\n");
                        exit(-1);
                    }

                    strcpy(buf, "0.0.0.0");
                    send_status = send(tcp_newsockfd, buf, 100, 0);
                    if(send_status < 0) {
                        perror("Send failed from server\n");
                        exit(-1);
                    }
                }
                printf("Successfully sent IPs\n");
            }
            continue;
        }

        if(FD_ISSET(udp_sockfd, &myfd)) {
            // The udp server (iterative)

            memset(buf, '\0', sizeof(buf));
            // Receiving the domain name
            int recv_status = recvfrom(udp_sockfd, (char *)buf, 100, 0, (struct sockaddr *) &udp_cliaddr, &udp_clilen);
            // Error checking
            if(recv_status < 0) {
                perror("Error in receiving\n");
                exit(-1);
            }
            printf("Received: %s\n", buf);

            struct hostent* resp = gethostbyname(buf);
            if(resp != NULL) {
                int tot_ips= 0;
                for(; resp->h_addr_list[tot_ips] != NULL; tot_ips++);

                int send_status = sendto(udp_sockfd, &tot_ips, sizeof(int), 0, (struct sockaddr *)&udp_cliaddr, udp_clilen);
                if(send_status < 0) {
                    perror("Send failed!\n");
                    exit(-1);
                }

                for(int i = 0; i < tot_ips; i++) {
                    memset(buf, '\0', sizeof(buf2));
                    strcpy(buf, inet_ntoa(*((struct in_addr *)resp->h_addr_list[i])));
                    int send_status = sendto(udp_sockfd, buf, 100, 0, (struct sockaddr *) &udp_cliaddr, udp_clilen);
                    if(send_status < 0) {
                        perror("Send failed from server\n");
                        exit(-1);
                    }
                }

            } else {
                // Send -1 for error
                int tot_ips = -1;
                sendto(udp_sockfd, &tot_ips, sizeof(int), 0, (struct sockaddr *)&udp_cliaddr, udp_clilen);

                strcpy(buf, "0.0.0.0");
                int send_status = sendto(udp_sockfd, buf, 100, 0, (struct sockaddr *) &udp_cliaddr, udp_clilen);
                if(send_status < 0) {
                    perror("Send failed from server\n");
                    exit(-1);
                }
            }
            
            printf("Successfully sent IPs\n");
            continue;
        }

        if(FD_ISSET(tcp_sockfd, &myfd) == 0 && FD_ISSET(udp_sockfd, &myfd) == 0) {
            // timed out
            printf("Server timed out\n");
            break;
        }
    }

    // Close both sockets
    close(udp_sockfd);
    close(tcp_sockfd);

    return 0; 
}
