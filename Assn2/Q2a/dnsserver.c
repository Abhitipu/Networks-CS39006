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
#include <netdb.h>
  
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
    
    printf("Server started\n");

    fd_set myfd;
    FD_ZERO(&myfd);
    FD_SET(sockfd, &myfd);

    socklen_t len = sizeof(cliaddr);

    char buf[101], buf2[101]; 
    memset(buf2, '\0', sizeof(buf2));
    memset(buf, '\0', sizeof(buf));

    // The mainloop
    while(1) {
        // Reset buffer everytime
        memset(buf, '\0', sizeof(buf));

        // Waiting from the client's side : No time limit
        int select_status = select(sockfd + 1, &myfd, 0, 0, 0);
        if(select_status < 0) {
            perror("Error in select\n");
            exit(-1);
        }

        // Receiving the domain name
        int recv_status = recvfrom(sockfd, (char *)buf, 100, 0, (struct sockaddr *) &cliaddr, &len);
        // Error checking
        if(recv_status < 0) {
            perror("Error in receiving\n");
            exit(-1);
        }

        printf("Received: %s\n", buf);
        
        struct hostent* resp = gethostbyname(buf);
        if(resp != NULL) {
            strcpy(buf2, inet_ntoa(*((struct in_addr *)resp->h_addr_list[0])));
        } else {
            strcpy(buf2, "0.0.0.0");
        }
        
        int send_status = sendto(sockfd, buf2, 100, 0, (struct sockaddr *) &cliaddr, len);
        if(send_status < 0) {
            perror("Send failed from server\n");
            exit(-1);
        }
        printf("Successfully sent IP\n");
    }

    close(sockfd);
    return 0; 
}
