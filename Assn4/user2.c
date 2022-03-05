#include "rsocket.h"

const int LOCAL_PORT = 50000 + 2*(0005) + 1;
const int REMOTE_PORT = 50000 + 2*(0005);

int main() {
    struct sockaddr_in remoteAddr, localAddr; 
      //bind r_bind hoga? Haan
    // Create socket file descriptor 
    int sockfd = r_socket(AF_INET, SOCK_MRP, 0);
    if (sockfd < 0) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    // Reset variables
    memset(&remoteAddr, 0, sizeof(remoteAddr)); 
    memset(&localAddr, 0, sizeof(localAddr)); 
      
    // Local specifications
    localAddr.sin_family    = AF_INET; 
    localAddr.sin_addr.s_addr = INADDR_ANY; 
    localAddr.sin_port = htons(LOCAL_PORT); 
      
    // Bind the socket with the server address 
    if(r_bind(sockfd, (const struct sockaddr *)&localAddr, sizeof(localAddr)) < 0 ) { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }
    char buf[BUFFER_SIZE+1];

    while(1){
        bzero(buf, sizeof(buf));
        socklen_t addrlen = sizeof(remoteAddr); 
        if(r_recvfrom(sockfd, buf, BUFFER_SIZE, 0, (struct sockaddr*)&remoteAddr, &addrlen) < 0)
        {
            perror("r_recvfrom() fail");
        }
        printf("Buf Recvd : %s\n", buf);
    }
    return 0;
}