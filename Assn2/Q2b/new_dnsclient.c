// Client implementation using tcp
/** TCP CLIENT **/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

int main(int argc, char* argv[]) {
    // Creating the socket
	int	sockfd ;
	struct sockaddr_in	serv_addr;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(-1);
	}

    memset(&serv_addr, 0, sizeof(serv_addr)); 

    // Connecting to the server
	serv_addr.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);    // This will set the value
	serv_addr.sin_port	= htons(20000);
    
    // Alright so we need to delay connect here?
	if ((connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}

	char buf[101];
    memset(buf, '\0', sizeof(buf));

    printf("Enter domain name for obtaining ip: ");
    scanf("%s", buf);

    int send_status = send(sockfd, buf, 100, 0);
    if(send_status < 0) {
        perror("Error in send\n");
        exit(-1);
    }

    fd_set myfd;
    FD_ZERO(&myfd);
    FD_SET(sockfd, &myfd);

    struct timeval timer;
    timer.tv_sec = 2;
    timer.tv_usec = 0;

    int select_status = select(sockfd + 1, &myfd, 0, 0, &timer);

    int n_ips;
	int parse_status = recv(sockfd, &n_ips, sizeof(int), 0);
    if(parse_status < 0) {
        printf("Couldn't send file completely!\n");
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
            int recv_status = recv(sockfd, buf, 100, 0);
            // Error checking
            if(recv_status < 0) {
                perror("Error in receiving\n");
                exit(-1);
            }

            printf("%s\n", buf);
        }
    } else {
        printf("Invalid domain name entered!\n");

        int select_status = select(sockfd + 1, &myfd, 0, 0, &timer);
        if(select_status < 0) {
            perror("Error in select!\n");
            close(sockfd);
            exit(-1);
        }

        int parse_status = recv(sockfd, buf, 100, 0);
        if(parse_status < 0) {
            perror("Error in recv!\n");
            exit(-1);
        }
    }

	close(sockfd);
    // Now we will close it

	return 0;

}
