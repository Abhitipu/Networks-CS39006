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
    if (argc < 2) {
        printf("Input file not specified!\n");
        exit(-1);
    }

    // Creating the socket
	int	sockfd ;
	struct sockaddr_in	serv_addr;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(-1);
	}

    memset(&serv_addr, 0, sizeof(serv_addr)); 

    // Opening the file
    // O_RDONLY flag says that its a read only file
    int input_fd = open(argv[1], O_RDONLY);
    
    if(input_fd < 0) {
        printf("File not found\n");
        exit(-1);
    }

    // Connecting to the server
	serv_addr.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);    // This will set the value
	serv_addr.sin_port	= htons(20000);
    
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
    int cnt = 0;
    
    while(read(input_fd, buf, 100) > 0) {
        // send the buffer
	    int check = send(sockfd, buf, 100, 0);

        if(check == -1) {
            perror("Send failed!\n");
            exit(-1);
        }

        // printf("%d\n", ++cnt);
        printf("%s", buf);
        memset(buf, '\0', sizeof(buf));
    }

    printf("Done\n");
    
	int parse_status = recv(sockfd, buf, 100, 0);
    if(parse_status < 0) {
        printf("Couldn't send file completely!\n");
        exit(-1);
    }
    
    printf("%s\n", buf);

    int nWords, nChars, nSentences;
    int word_status = recv(sockfd, &nWords, sizeof(nWords), 0);
    int char_status = recv(sockfd, &nChars, sizeof(nChars), 0);
    int sent_status = recv(sockfd, &nSentences, sizeof(nSentences), 0);

    printf("Words: %d\n", nWords);
    printf("Characters: %d\n", nChars);
    printf("Sentences: %d\n", nSentences);
		
	close(sockfd);
    // Now we will close it

	return 0;

}
