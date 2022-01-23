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
    if(send_status < 0) {
        perror("Error in send\n");
        exit(-1);
    }

    // Wait for some response from server end
    int select_status = select(sockfd + 1, &myfd, 0, 0, &timer);
    if(select_status < 0) {
        perror("Error in select\n");
        exit(-1);
    }

    if(timer.tv_sec == 0 && timer.tv_usec == 0) {
        printf("Connection timed out!\n");
        exit(-1);
    }

    int n_ips = 0;
    int recv_status = recvfrom(sockfd, &n_ips, sizeof(int), 0, (struct sockaddr *) &servaddr, &len);
    // Error checking
    if(recv_status < 0) {
        perror("Error in receiving\n");
        exit(-1);
    }

    if(n_ips != -1) {
        printf("Obtained %d ip addresses\n", n_ips);
        for(int i = 0; i < n_ips; i++) {
            timer.tv_sec = 2;
            timer.tv_usec = 0;

            int select_status = select(sockfd + 1, &myfd, 0, 0, &timer);
            if(select_status < 0) {
                perror("Error in select\n");
                exit(-1);
            }

            if(timer.tv_sec == 0 && timer.tv_usec == 0) {
                printf("Connection timed out!\n");
                exit(-1);
            }
            
            memset(buf, '\0', sizeof(buf));
            int recv_status = recvfrom(sockfd, buf, 100, 0, (struct sockaddr *) &servaddr, &len);
            // Error checking
            if(recv_status < 0) {
                perror("Error in receiving\n");
                exit(-1);
            }

            printf("%s\n", buf);
        }
    } else {
        printf("Invalid domain name entered!\n");

        timer.tv_sec = 2;
        timer.tv_usec = 0;

        int select_status = select(sockfd + 1, &myfd, 0, 0, &timer);
        if(select_status < 0) {
            perror("Error in select\n");
            exit(-1);
        }

        if(timer.tv_sec == 0 && timer.tv_usec == 0) {
            printf("Connection timed out!\n");
            exit(-1);
        }

        memset(buf, '\0', sizeof(buf));
        int recv_status = recvfrom(sockfd, buf, 100, 0, (struct sockaddr *) &servaddr, &len);
        // Error checking
        if(recv_status < 0) {
            perror("Error in receiving\n");
            exit(-1);
        }
        
        printf("%s\n", buf);
    }

    close(sockfd); 
    return 0; 
} 
