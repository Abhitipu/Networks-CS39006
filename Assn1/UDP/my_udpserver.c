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
        int cur_len;
        int nWords, nChars, nSentences;
        int lastWasLetter = 0;
        nWords = nChars = nSentences = 0;

        while(1) {
            // Reset buffer everytime
            memset(buf, '\0', sizeof(buf));
            // Receive a max of 100 characters at a time
		    int check = recvfrom(sockfd, (char *)buf, 100, 0, (struct sockaddr *) &cliaddr, &len);

            // Error checking
            if(check == -1) {
                perror("Error in receiving\n");
                exit(-1);
            }

            cur_len = strlen(buf);
            // End of stream
            if(cur_len == 0)
                break;
            printf("%s", buf);

            // Main program logic
            for(int i = 0; i < cur_len && buf[i] != '\0'; i++) {
                if(buf[i] == ' ' || buf[i] == '.') {
                    // If the last one was a letter, we have a new word
                    if(lastWasLetter)
                        nWords++;

                    lastWasLetter = 0;
                    nSentences += (buf[i] == '.');
                }
                else {
                    // Alphanumeric
                    if(buf[i] != '\n')
                        lastWasLetter = 1;
                    else if(lastWasLetter)
                        lastWasLetter = 0, nWords++;

                }
                // Always increment nChars
                nChars++;
            }
        }
        printf("Done\n");

        nSentences--;

        memset(buf, '\0', sizeof(buf));
        strcpy(buf, "Parsing complete! Returning results\n");
        sendto(sockfd, buf, 100, 0, (struct sockaddr *) &cliaddr, len);

        // Returning the results
        sendto(sockfd, &nWords, sizeof(nWords), 0, (struct sockaddr *) &cliaddr, len);
        sendto(sockfd, &nChars, sizeof(nChars), 0, (struct sockaddr *) &cliaddr, len);
        sendto(sockfd, &nSentences, sizeof(nSentences), 0, (struct sockaddr *) &cliaddr, len);
    }
    close(sockfd);
    return 0; 
}
