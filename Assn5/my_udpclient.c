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
    // Creating the socket
	int	sockfd ;
	struct sockaddr_in	servaddr, cliaddr;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(-1);
	}
  
    memset(&servaddr, 0, sizeof(servaddr)); 

    // // Server information.. we dont explicitly call connect() here
    // servaddr.sin_family = AF_INET; 
    // inet_aton("127.0.0.1", &servaddr.sin_addr); 
    // servaddr.sin_port = htons(8181); 
    

    cliaddr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &cliaddr.sin_addr); 
    cliaddr.sin_port = htons(32164); 
    // Initial communication 
    int n;
    socklen_t len = sizeof(servaddr); 
    char buf[101] = "Hello from client!\n";
    // sendto(sockfd, (const char *)buf, 100, 0, (struct sockaddr *) &servaddr, len); 

     // Bind the socket with the client address 
    if(bind(sockfd, (const struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0 ) { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    
    recvfrom(sockfd, buf, 100, 0, (struct sockaddr *) &servaddr, &len);
    printf("%s\n", buf);

    // // Reading from file using the read api 
    // while(1) {
    //     // send the buffer
    //     int sz = read(input_fd, buf, 100);
	//     int check = sendto(sockfd, buf, 100, 0, (struct sockaddr *) &servaddr, len);

    //     // End of stream
    //     if(sz == 0)
    //         break;

    //     // Error checking
    //     if(check == -1) {
    //         perror("Send failed!\n");
    //         exit(-1);
    //     }

    //     // printf("%d\n", ++cnt);
    //     printf("%s", buf);
    //     memset(buf, '\0', sizeof(buf));
    // }

    // printf("Done\n");
    
    // // Error checking
    // int parse_status = recvfrom(sockfd, buf, 100, 0, (struct sockaddr *) &servaddr, &len);
    // if(parse_status < 0) {
    //     printf("Couldn't send file completely!\n");
    //     exit(-1);
    // }
    
    // printf("%s\n", buf);

    // // Getting back the results
    // int nWords, nChars, nSentences;
    // int word_status = recvfrom(sockfd, &nWords, sizeof(nWords), 0, (struct sockaddr *) &servaddr, &len);
    // int char_status = recvfrom(sockfd, &nChars, sizeof(nChars), 0, (struct sockaddr *) &servaddr, &len);
    // int sent_status = recvfrom(sockfd, &nSentences, sizeof(nSentences), 0, (struct sockaddr *) &servaddr, &len);

    // if(word_status < 0 || char_status < 0 || sent_status < 0) {
    //     perror("Error in getting results");
    //     exit(-1);
    // }

    // printf("Words: %d\n", nWords);
    // printf("Characters: %d\n", nChars);
    // printf("Sentences: %d\n", nSentences);
		
    close(sockfd); 
    return 0; 
} 
