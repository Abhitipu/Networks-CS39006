#include "rsocket.h"

const int LOCAL_PORT = 50000 + 2*(0005);
const int REMOTE_PORT = 50000 + 2*(0005) + 1;

int main() {
	struct sockaddr_in	mrpLocaladdr, mrpRemoteaddr;

    // Create mrp socket
    int mrpSockfd = r_socket(AF_INET, SOCK_MRP, 0);
	if (mrpSockfd < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}

    // Reset variables
    memset(&mrpLocaladdr, 0, sizeof(mrpLocaladdr));
    memset(&mrpRemoteaddr, 0, sizeof(mrpRemoteaddr));
      
    // All this is server info
	mrpRemoteaddr.sin_family		= AF_INET;
	mrpRemoteaddr.sin_addr.s_addr	= INADDR_ANY;   
	mrpRemoteaddr.sin_port		= htons(REMOTE_PORT);     // Network Byte Order or big endian system

    // Makefile bana lete.. 
    // dropmessage se pehle test bhi ho jaayega
    // Bind the mrp socket with the server address 
	if (r_bind(mrpSockfd, (struct sockaddr *) &mrpRemoteaddr, sizeof(mrpRemoteaddr)) < 0) {
		perror("Unable to bind local address\n");
		exit(0);
	}

    char buf[BUFFER_SIZE + 1];
    bzero(buf, sizeof(buf));
    printf("Enter message to be sent: ");
    scanf(" %[^\n]", buf);

    socklen_t len = sizeof(mrpRemoteaddr);
    for(int i = 0; i < strlen(buf); i++) {
        char tempbuf[2];
        tempbuf[0] = buf[i];
        tempbuf[1] = '\0';
        int sendStatus = r_sendto(mrpSockfd, tempbuf, 1, 0, (const struct sockaddr*)&mrpRemoteaddr, len); 
        if(sendStatus < 0) {
            perror("Send failed!\n");
            exit(-1);
        }
        printf("Sent %c\n", buf[i]);
    }
    return 0;
}