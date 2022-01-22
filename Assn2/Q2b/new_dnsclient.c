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

    // Reading from input file
	char buf[101];
    memset(buf, '\0', sizeof(buf));

    // We will receive a notification when connected to server!
	recv(sockfd, buf, 100, 0);
	printf("%s\n", buf);
    
	int parse_status = recv(sockfd, buf, 100, 0);
    if(parse_status < 0) {
        printf("Couldn't send file completely!\n");
        exit(-1);
    }
    
    printf("%s\n", buf);

	close(sockfd);
    // Now we will close it

	return 0;

}
