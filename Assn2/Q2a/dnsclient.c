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
    
    // Creating the socket
	int	sockfd ;
	struct sockaddr_in	servaddr;
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

    // Do the 2000ms thing here : once connection established, no probs
    char buf[101] = "Hello from client!\n";
    sendto(sockfd, (const char *)buf, 100, 0, (struct sockaddr *) &servaddr, len); 

    int init_status = recvfrom(sockfd, buf, 100, 0, (struct sockaddr *) &servaddr, &len);

    printf("%s\n", buf);

    printf("Enter domain name to obtain ip address: ");
    memset(buf, '\0', sizeof(buf));
    scanf(" %s", buf);

    sendto(sockfd, (const char *)buf, 100, 0, (struct sockaddr *) &servaddr, len); 
    // Send buf to server

    printf("Done\n");
    
    // Error checking
    int parse_status = recvfrom(sockfd, buf, 100, 0, (struct sockaddr *) &servaddr, &len);
    if(parse_status < 0) {
        printf("Couldn't send domain name\n");
        exit(-1);
    }
    
    printf("%s\n", buf);

    close(sockfd); 
    return 0; 
} 
