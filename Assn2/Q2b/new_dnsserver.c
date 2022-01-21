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
  
int main() { 
    int sockfd; 
    struct sockaddr_in servaddr, cliaddr; 
      
    // Create socket file descriptor 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    // Reset variables
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
      
    // Server specifications
    servaddr.sin_family    = AF_INET; 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(8181); 
      
    // Bind the socket with the server address 
    if(bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ) { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    

    socklen_t len = sizeof(cliaddr);
    char buf[101]; 
    memset(buf, '\0', sizeof(buf));

    // Receive message from a client
	recvfrom(sockfd, (char *)buf, 100, 0, (struct sockaddr *) &cliaddr, &len);
    printf("%s", buf);

    memset(buf, '\0', sizeof(buf));
    strcpy(buf, "Hello from server!\n");
    sendto(sockfd, buf, 100, 0, (struct sockaddr *) &cliaddr, len);
 
    len = sizeof(cliaddr);

    // The mainloop
    while(1) {
        // Reset buffer everytime
        memset(buf, '\0', sizeof(buf));
        // Receiving the domain name
        int check = recvfrom(sockfd, (char *)buf, 100, 0, (struct sockaddr *) &cliaddr, &len);

        // Error checking
        if(check == -1) {
            perror("Error in receiving\n");
            exit(-1);
        }
        
        printf("%s", buf);
        
        // call the gethostbyname method and then return the results here
        memset(buf, '\0', sizeof(buf));

        strcpy(buf, "Parsing complete! Returning results\n");
        sendto(sockfd, buf, 100, 0, (struct sockaddr *) &cliaddr, len);
    }

    close(sockfd);
    return 0; 
}

/*
tcp server implementation: need to integrate

int main() {
    // The required socket descriptors
	int	sockfd, newsockfd ; 
	int	clilen;
    // Here we need to store the client information also
	struct sockaddr_in	cli_addr, serv_addr;


    // Creating the socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}

    // All this is server info
	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;   
	serv_addr.sin_port		= htons(20000);     // Network Byte Order or big endian system

    //  We will bind it to a port on our machine
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        // There is a global variable called errno which gets set in case of an error
		perror("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5);  //  5 concurrent connections

	char buf[101];		
	while (1) {
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

		if (newsockfd < 0) {
			perror("Accept error\n");
			exit(0);
		}

        // After connection we send a success message
		strcpy(buf,"Successfully connected to server\n");
		send(newsockfd, buf, strlen(buf) + 1, 0);

        printf("Done\n");

        memset(buf, '\0', sizeof(buf));
        strcpy(buf, "Parsing complete! Returning results\n");
        send(newsockfd, buf, 100, 0);

		close(newsockfd);           // It is always a good idea to close the newsockfd
	}
	return 0;
}
int main() {
    // The required socket descriptors
	int	sockfd, newsockfd ; 
	int	clilen;
    // Here we need to store the client information also
	struct sockaddr_in	cli_addr, serv_addr;


    // Creating the socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}

    // All this is server info
	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;   
	serv_addr.sin_port		= htons(20000);     // Network Byte Order or big endian system

    //  We will bind it to a port on our machine
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        // There is a global variable called errno which gets set in case of an error
		perror("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5);  //  5 concurrent connections

	char buf[101];		
	while (1) {
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

		if (newsockfd < 0) {
			perror("Accept error\n");
			exit(0);
		}

        // After connection we send a success message
		strcpy(buf,"Successfully connected to server\n");
		send(newsockfd, buf, strlen(buf) + 1, 0);

        printf("Done\n");

        memset(buf, '\0', sizeof(buf));
        strcpy(buf, "Parsing complete! Returning results\n");
        send(newsockfd, buf, 100, 0);

		close(newsockfd);           // It is always a good idea to close the newsockfd
	}
	return 0;
}
 * 
 */
