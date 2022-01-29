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

#define BUFFER_SIZE 101

#define START 0
#define OPENED 1
#define GOT_USER 2
#define AUTHENTICATED 3
#define QUIT 4

#define SUCCESS "200"
#define ERROR1 "500"
#define ERROR2 "600"

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

        // check for quit
        if(strcmp(buf, "quit") == 0) {
            state = QUIT;
        }

        switch (state) {           
            case START: {
                // open ip port
                
                memset(port, '\0', sizeof(port));
                memset(ip, '\0', sizeof(ip));
                
                int ret = sscanf(buf,"open %s %s", ip, port);
                
                // printf("ret = %d\n", ret);
                printf("Ip : %s\n", ip);
                printf("Port : %s\n", port);

                if(ret < 2) {
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
                // send username
                printf("Inside opened!\n");

                int send_status = send(sockfd, buf, BUFFER_SIZE - 1, 0);
                if(send_status < 0) {
                    perror("Error in send\n");
                    exit(-1);
                }

                printf("Sent to server\n");

                bzero(buf, sizeof(buf));
                int parse_status = recv(sockfd, buf, BUFFER_SIZE, 0);
                if(parse_status < 0) {
                    printf("Couldn't send file completely!\n");
                    exit(-1);
                }

                printf("Received from server\n");

                if(strcmp(buf, SUCCESS) == 0) {
                    printf("Matched username\n");
                    state = GOT_USER;
                } else if (strcmp(buf, ERROR2) == 0) {
                    printf("Incorrect command sent!\n");
                } else if (strcmp(buf, ERROR1) == 0) {
                    printf("Username doesn't exist!\n");
                } else {
                    printf("Something's not right in username!\n");
                    exit(-1);
                }
                break;
            }

            case GOT_USER: {
                // send password
                int send_status = send(sockfd, buf, BUFFER_SIZE - 1, 0);
                if(send_status < 0) {
                    perror("Error in send\n");
                    exit(-1);
                }

                bzero(buf, sizeof(buf));
                int parse_status = recv(sockfd, buf, BUFFER_SIZE, 0);
                if(parse_status < 0) {
                    printf("Couldn't send file completely!\n");
                    exit(-1);
                }

                if(strcmp(buf, SUCCESS) == 0) {
                    printf("Matched password\n");
                    state = AUTHENTICATED;
                } else if (strcmp(buf, ERROR2) == 0) {
                    printf("Incorrect command sent!\n");
                    state = OPENED;
                } else if (strcmp(buf, ERROR1) == 0) {
                    printf("Password doesn't match!\n");
                    state = OPENED;
                } else {
                    printf("Something's not right in password!\n");
                    exit(-1);
                }

                break;
            }

            case AUTHENTICATED: {
                // commands 4-10 are executed here
                int send_status = send(sockfd, buf, BUFFER_SIZE - 1, 0);
                if(send_status < 0) {
                    perror("Error in send\n");
                    exit(-1);
                }

                if(strcmp(buf, "dir") == 0) {
                    bzero(buf, sizeof(buf));
                    int parse_status = recv(sockfd, buf, BUFFER_SIZE, 0);
                    if(parse_status < 0) {
                        printf("Couldn't send file completely!\n");
                        exit(-1);
                    }
                    char *b = buf;
                    while(strlen(b) > 0)
                    {
                        printf("%s\n", b);
                        b = b + strlen(b) + 1;
                    }
                }
                /* code
                get all commands
                */
                break;
            }
            case QUIT: {
                // send quit command to server!
                int send_status = send(sockfd, buf, BUFFER_SIZE - 1, 0);
                if(send_status < 0) {
                    perror("Error in send\n");
                    exit(-1);
                }

                // and close our socket
            	close(sockfd);
                break;
            }
            default:
                break;
        }
    }
	return 0;
}