/** TCP SERVER **/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>

int main() {
    // The required socket descriptors
	int	sockfd, newsockfd ; 
	int	clilen;
    // Here we need to store the client information also
	struct sockaddr_in	cli_addr, serv_addr;


    // Creating the socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}

    // All this is server info
	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;   
	serv_addr.sin_port		= htons(20000);     // Network Byte Order or big endian system

    //  We will bind it to a port on our machine
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        // There is a global variable called errno which gets set in case of an error
		perror("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5);  //  5 concurrent connections

	char buf[101];		
	while (1) {
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

		if (newsockfd < 0) {
			perror("Accept error\n");
			exit(0);
		}

        // After connection we send a success message
		strcpy(buf,"Successfully connected to server\n");
		send(newsockfd, buf, strlen(buf) + 1, 0);

        // So we will need to play with the return value of recv?
        int len;
        int nWords, nChars, nSentences;
        int lastWasLetter = 0;
        nWords = nChars = nSentences = 0;

        while(1) {
            memset(buf, '\0', sizeof(buf));

		    int check = recv(newsockfd, buf, 100, 0);

            if(check == -1) {
                perror("Error in receiving\n");
                exit(-1);
            }

            // end of stream
            len = strlen(buf);
            if(len == 0)
                break;

		    printf("%s", buf);

            // The main logic
            for(int i = 0; i < len && buf[i] != '\0'; i++) {
                if(buf[i] == ' ' || buf[i] == '.') {
                    if(lastWasLetter)
                        nWords++;

                    lastWasLetter = 0;
                    // counting '.' is enough for counting sentences
                    nSentences += (buf[i] == '.');
                }
                else {
                    // Alphanumeric logic
                    if(buf[i] != '\n')
                        lastWasLetter = 1;
                    else if(lastWasLetter)
                        lastWasLetter = 0, nWords++;
                }
                // Always incremented
                nChars++;
            }
        }

        printf("Done\n");

        memset(buf, '\0', sizeof(buf));
        strcpy(buf, "Parsing complete! Returning results\n");
        send(newsockfd, buf, 100, 0);

        // Finally sending the results
        send(newsockfd, &nWords, sizeof(nWords), 0);
        send(newsockfd, &nChars, sizeof(nChars), 0);
        send(newsockfd, &nSentences, sizeof(nSentences), 0);

		close(newsockfd);           // It is always a good idea to close the newsockfd
	}
	return 0;
}
