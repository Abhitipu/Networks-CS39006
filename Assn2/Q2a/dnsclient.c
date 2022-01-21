// take a host name
// send to the server using the udp socket
// Get the ip from the server and print it
// if 0.0.0.0 : exit with message
// if server is busy : wait for 2 seconds and close with message

/** THE UDP CLIENT **/
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 

#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <fcntl.h>
  
int main(int argc, char* argv[]) { 
    
    char buf[101];
    memset(buf, '\0', sizeof(buf));
    printf("Enter domain name to obtain ip address: ");
    scanf("%s", buf);

    // Creating the socket
	int	sockfd;
	struct sockaddr_in servaddr;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(-1);
	}
  
    memset(&servaddr, 0, sizeof(servaddr)); 

    // Server information.. we dont explicitly call connect() here
    servaddr.sin_family = AF_INET; 
    inet_aton("127.0.0.1", &servaddr.sin_addr); 
    servaddr.sin_port = htons(8181); 
    
    // Initial communication 
    int n;
    socklen_t len = sizeof(servaddr); 

    fd_set myfd;
    FD_ZERO(&myfd);
    FD_SET(sockfd, &myfd);

    struct timeval timer;
    timer.tv_sec = 2;
    timer.tv_usec = 0;

    int send_status = sendto(sockfd, (const char *)buf, 100, 0, (struct sockaddr *) &servaddr, len); 

    // Wait for some response from server end
    int select_status = select(sockfd + 1, &myfd, 0, 0, &timer);

    if(timer.tv_sec == 0 && timer.tv_usec == 0) {
        printf("Connection timed out!\n");
        exit(-1);
    }

    int recv_status = recvfrom(sockfd, buf, 100, 0, (struct sockaddr *) &servaddr, &len);
    // Error checking
    if(recv_status == -1) {
        perror("Error in receiving\n");
        exit(-1);
    }

    printf("Obtained ip address is: %s\n", buf);
    if(strcmp(buf, "0.0.0.0") == 0) {
        printf("Invalid domain name entered!\n");
    }

    close(sockfd); 
    return 0; 
} 
