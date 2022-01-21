// Get a domain name from the client
// Return corresponding ip address using the gethostbyname method
// Assume iterative server
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
