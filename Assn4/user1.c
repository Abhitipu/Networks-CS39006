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

    // Local
    mrpLocaladdr.sin_family         = AF_INET;
    inet_aton("127.0.0.1", &mrpLocaladdr.sin_addr); 
    mrpLocaladdr.sin_port           = htons(LOCAL_PORT);

    // All this is server info
	mrpRemoteaddr.sin_family		= AF_INET;
	inet_aton("127.0.0.1", &mrpRemoteaddr.sin_addr);   
	mrpRemoteaddr.sin_port		= htons(REMOTE_PORT);     // Network Byte Order or big endian system

    // Bind the mrp socket with the server address 
	if (r_bind(mrpSockfd, (struct sockaddr *) &mrpLocaladdr, sizeof(mrpLocaladdr)) < 0) {
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
        int sendStatus = r_sendto(mrpSockfd, tempbuf, 2, 0, (const struct sockaddr*)&mrpRemoteaddr, len); 
        if(sendStatus < 0) {
            perror("Send failed!\n");
            exit(-1);
        }
        printf("Sent %c\n", buf[i]);
    }

    // So that r_close() is never called
    while(1);

    r_close(mrpSockfd);
    return 0;
}