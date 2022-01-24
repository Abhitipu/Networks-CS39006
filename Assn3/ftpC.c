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
#include <regex.h>

#define PORT 20000
#define BUFFER_SIZE 101

#define START 0
#define OPENED 1
#define GOT_USER 2
#define AUTHENTICATED 3
#define QUIT 4

int count(char* inp, char toSearch) {
    if(inp == NULL)
        return 0;

    int n = strlen(inp);
    int ans = 0;
    for(int i = 0; i < n; i++)
        ans += (inp[i] == toSearch);
    
    return ans;
}

int main(int argc, char* argv[]) {
    // Creating the socket
	int	sockfd ;
	struct sockaddr_in	serv_addr;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(-1);
	}

    char port[100];
    char ip[100];
    char buf[BUFFER_SIZE];
    
    int state = START;
    while(state != QUIT) {
        printf("myFTP>");
        scanf(" %[^\n]", buf);
        printf("Got : %s\n", buf);

        switch (state) {           
            case START: {
                // open ip port
                
                memset(port, '\0', sizeof(port));
                memset(ip, '\0', sizeof(ip));
                
                int ret = sscanf(buf,"open %s %s", ip, port);

                printf("ret = %d\n", ret);
                printf("Ip : %s\n", ip);
                printf("Port : %s\n", port);

                if(ret < 2 || count(buf, ' ') > 2) {
                    printf("len(buf) = %ld\n", strlen(buf));
                    printf("Incorrect command format!\n");
                    printf("Expected format is: ?\n");
                    break;
                }

                // also check for valid ip address and port: will connect do it for us? 

                memset(&serv_addr, 0, sizeof(serv_addr)); 
                serv_addr.sin_family	= AF_INET;
                inet_aton(ip, &serv_addr.sin_addr);    // This will set the value
                serv_addr.sin_port	= htons(atoi(port));

                // Connecting to the server
                if ((connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0) {
                    perror("Unable to connect to server\n");
                    exit(0);
                }

                state = OPENED;
                break;
            }

            case OPENED: {
                /*
                int send_status = send(sockfd, buf, BUFFER_SIZE - 1, 0);
                if(send_status < 0) {
                    perror("Error in send\n");
                    exit(-1);
                }
                // fd_set for select()

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
                get user and check
                */
                break;

            }
            case GOT_USER: {
                /*
                get password and check
                */
                break;
                
            }
            case AUTHENTICATED: {

                /* code
                get all commands
                */
                break;
            }
            case QUIT: {
                // Now we will close it
            	close(sockfd);
                break;
            }
            default:
                break;
        }
    }
	return 0;
}