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

// A wrapper function for the gethostbyname api
void getHostNameWrapper(char *buf) {
    struct hostent* resp = gethostbyname(buf);
    memset(buf, '\0', sizeof(buf));
    if(resp != NULL) {
        strcpy(buf, inet_ntoa(*((struct in_addr *)resp->h_addr_list[0])));
    } else {
        strcpy(buf, "0.0.0.0");
    }

    return;
}
  
int main() { 
    int udp_sockfd; 
	int	tcp_sockfd, tcp_newsockfd;

    struct sockaddr_in udp_servaddr, udp_cliaddr; 
	struct sockaddr_in	tcp_cli_addr, tcp_serv_addr;

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

    memset(&tcp_cli_addr, 0, sizeof(tcp_cli_addr));
    memset(&tcp_serv_addr, 0, sizeof(tcp_serv_addr));
      
    // Server specifications
    udp_servaddr.sin_family    = AF_INET; 
    udp_servaddr.sin_addr.s_addr = INADDR_ANY; 
    udp_servaddr.sin_port = htons(8181); 
      
    // All this is server info
	tcp_serv_addr.sin_family		= AF_INET;
	tcp_serv_addr.sin_addr.s_addr	= INADDR_ANY;   
	tcp_serv_addr.sin_port		= htons(20000);     // Network Byte Order or big endian system

    // Bind the socket with the server address 
    if(bind(udp_sockfd, (const struct sockaddr *)&udp_servaddr, sizeof(udp_servaddr)) < 0 ) { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 

    //  We will bind it to a port on our machine
	if (bind(tcp_sockfd, (struct sockaddr *) &tcp_serv_addr, sizeof(tcp_serv_addr)) < 0) {
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

    // TODO: change len to udp_clilen
    socklen_t len = sizeof(udp_cliaddr);
	socklen_t tcp_clilen = sizeof(tcp_cli_addr);

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
            tcp_newsockfd = accept(tcp_sockfd, (struct sockaddr *) &tcp_cli_addr, &tcp_clilen);

            if (tcp_newsockfd < 0) {
                perror("Accept error\n");
                exit(0);
            }

            if(fork() == 0) {
                // child: Need to figure this out
                /*
                send(tcp_newsockfd, buf, strlen(buf) + 1, 0);
                memset(buf, '\0', sizeof(buf));
                strcpy(buf, "Parsing complete! Returning results\n");
                send(tcp_newsockfd, buf, 100, 0);

                close(tcp_newsockfd);
                */
            }
            continue;
        }

        if(FD_ISSET(udp_sockfd, &myfd)) {
            // The udp server (iterative)

            // Receiving the domain name
            int recv_status = recvfrom(udp_sockfd, (char *)buf, 100, 0, (struct sockaddr *) &udp_cliaddr, &len);
            // Error checking
            if(recv_status < 0) {
                perror("Error in receiving\n");
                exit(-1);
            }
            printf("Received: %s\n", buf);
            getHostNameWrapper(buf);

            int send_status = sendto(udp_sockfd, buf, 100, 0, (struct sockaddr *) &udp_cliaddr, len);
            if(send_status < 0) {
                perror("Send failed from server\n");
                exit(-1);
            }
            printf("Successfully sent IP\n");
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

/*
tcp server implementation: need to integrate

int main() {
    // The required socket descriptors
	int	tcp_sockfd, tcp_newsockfd ; 
	int	tcp_clilen;
    // Here we need to store the client information also
	struct sockaddr_in	tcp_cli_addr, tcp_serv_addr;


    // Creating the socket
	if ((tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}

    // All this is server info
	tcp_serv_addr.sin_family		= AF_INET;
	tcp_serv_addr.sin_addr.s_addr	= INADDR_ANY;   
	tcp_serv_addr.sin_port		= htons(20000);     // Network Byte Order or big endian system

    //  We will bind it to a port on our machine
	if (bind(tcp_sockfd, (struct sockaddr *) &tcp_serv_addr, sizeof(tcp_serv_addr)) < 0) {
        // There is a global variable called errno which gets set in case of an error
		perror("Unable to bind local address\n");
		exit(0);
	}

	listen(tcp_sockfd, 5);  //  5 concurrent connections

	char buf[101];		
	while (1) {
		tcp_clilen = sizeof(tcp_cli_addr);
		tcp_newsockfd = accept(tcp_sockfd, (struct sockaddr *) &tcp_cli_addr, &tcp_clilen);

		if (tcp_newsockfd < 0) {
			perror("Accept error\n");
			exit(0);
		}

        // After connection we send a success message
		strcpy(buf,"Successfully connected to server\n");
		send(tcp_newsockfd, buf, strlen(buf) + 1, 0);

        printf("Done\n");

        memset(buf, '\0', sizeof(buf));
        strcpy(buf, "Parsing complete! Returning results\n");
        send(tcp_newsockfd, buf, 100, 0);

		close(tcp_newsockfd);           // It is always a good idea to close the tcp_newsockfd
	}
	return 0;
}
 * 
 */
