/*    THE CLIENT PROCESS */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// these are the required imports
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Input file not specified!\n");
        exit(-1);
    }

    char* infile_name = argv[1];
    FILE* infile_ptr = fopen(infile_name, "r");
    
    if(infile_ptr == NULL) {
        printf("File not found\n");
        exit(-1);
    }
    

	int	sockfd ;
    // So this is serv_addr ... but why : because you will connect to it lmao
	struct sockaddr_in	serv_addr;

	int i;
	char buf[100];

	for(i=0; i < 100; i++) 
        buf[i] = '\0';

    read();

	/* Opening a socket is exactly similar to the server process */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}

	/* Recall that we specified INADDR_ANY when we specified the server
	   address in the server. Since the client can run on a different
	   machine, we must specify the IP address of the server. 

	   In this program, we assume that the server is running on the
	   same machine as the client. 127.0.0.1 is a special address
	   for "localhost" (this machine)
	   
	    IF YOUR SERVER RUNS ON SOME OTHER MACHINE, YOU MUST CHANGE 
        THE IP ADDRESS SPECIFIED BELOW TO THE IP ADDRESS OF THE 
        MACHINE WHERE YOU ARE RUNNING THE SERVER. 
    */

	serv_addr.sin_family	= AF_INET;
    // So the serv addr has the ip and the port
	inet_aton("127.0.0.1", &serv_addr.sin_addr);    // This will set the value
	serv_addr.sin_port	= htons(20000);
    // We could have zeroed the rest of the struct?
    
    // Here we also dont bother about choosing bind
    // We dont care about which port we are using.... 
    // The kernel will automatically assign a port to us

	/* With the information specified in serv_addr, the connect()
	   system call establishes a connection with the server process.
	*/
    //  We typecast at the last moment
	if ((connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}

	/* 
       After connection, the client can send or receive messages.
	   However, please note that recv() will block when the
	   server is not sending and vice versa. Similarly send() will
	   block when the server is not receiving and vice versa. For
	   non-blocking modes, refer to the online man pages.
	*/


    // So we will receive a notification when connected to server!
	recv(sockfd, buf, 100, 0);
	printf("%s\n", buf);

	
	strcpy(buf,"Message from client");
    // That last + 1 is for the null zero?
	send(sockfd, buf, strlen(buf) + 1, 0);
		
	close(sockfd);
    // Now we will close it

    fclose(infile_ptr);
    // close the input file

	return 0;

}
