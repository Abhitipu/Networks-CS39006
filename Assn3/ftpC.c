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
#include <dirent.h>

// Client fsm States 
#define START 0
#define OPENED 1
#define GOT_USER 2
#define AUTHENTICATED 3
#define QUIT 4

// Status Codes
#define SUCCESS "200"
#define ERROR1 "500"
#define ERROR2 "600"

// Buffer size
#define BUFFER_SIZE 200

// Custom Minimum function
int Min(int a, int b) {
    return (a < b) ? a : b;
}

// get remote_file local_file
int receive_file(int sockfd, char* buf) {
    // local_file to remote_file
    // Extact data from command
    char remote_file[BUFFER_SIZE + 1], local_file[BUFFER_SIZE + 1];
    int check = sscanf(buf, "get %s %s", remote_file, local_file);
    if(check < 2) {
        printf("Invalid format for get!\n");
        printf("Expected format is: get <remote_file> <local_file>\n");
        return -1;
    }

    // open the local_file
    int get_fd = open(local_file, O_WRONLY | O_CREAT | O_TRUNC);
    if(get_fd < 0) {
        printf("Client can't open the file for writing!\n");
        return -1;
    }

    // send iff we can open local_file (command)
    int send_status = send(sockfd, buf, strlen(buf) + 1, 0);
    if(send_status < 0) {
        perror("Error in send\n");
        exit(-1);
    }

    // check if server can open remote file
    bzero(buf, sizeof(buf));
    int parse_status = recv(sockfd, buf, 4, 0);
    if(parse_status < 0) {
        printf("Couldn't send file completely!\n");
        exit(-1);
    }
    
    // server can open remote file : receive block by block
    if(strcmp(buf, SUCCESS) == 0) {
        char flag = 'M';
        while(flag == 'M'){
            bzero(buf, sizeof(buf));
            int parse_status = recv(sockfd, buf, 3, 0);
            if(parse_status < 0) {
                perror("Error in recv!\n");
                exit(-1);
            }

            flag = buf[0];
    
            uint16_t nbytes;
            memcpy(&nbytes, buf + 1, 2);
            uint16_t len = ntohs(nbytes);

            for(uint16_t cur = 0; cur < len; cur += parse_status) {
                bzero(buf, sizeof(buf));
                parse_status = recv(sockfd, buf, Min(BUFFER_SIZE, len - cur), 0);
                if(parse_status < 0) {
                    perror("Error in recv!\n");
                    exit(-1);
                }
                // printf("Recv(%d) %s\n", parse_status, buf);
                if(write(get_fd, buf, parse_status) < 0)
                {
                    perror("can't write");
                    exit(1);
                }
            }
            if(flag == 'L')
            {
                // Last block encountered, exit
                break;
            }
        }
        close(get_fd);
        return 0;
    } else if (strcmp(buf, ERROR1) == 0) {
        printf("Error 500, File doesnt exist or can't be opened on server side!\n");
        return -1;
    }
}

int send_file(int sockfd, char* buf) {
    // remote_file to local_file
    // Extact data from command
    char remote_file[BUFFER_SIZE + 1], local_file[BUFFER_SIZE + 1];
    int check = sscanf(buf, "put %s %s", local_file, remote_file);
    if(check < 2) {
        printf("Invalid format for get!\n");
        printf("Expected format is: get <local_file> <remote_file>\n");
        return -1;
    }

    // open the local_file
    int put_fd = open(local_file, O_RDONLY);
    if(put_fd < 0) {
        perror("Client can't open the file for writing!\n");
        return -1;
    }

    // send iff we can open local_file
    int send_status = send(sockfd, buf, strlen(buf) + 1, 0);
    if(send_status < 0) {
        perror("Error in send\n");
        exit(-1);
    }

    // check if server can open remote file
    bzero(buf, sizeof(buf));
    int parse_status = recv(sockfd, buf, BUFFER_SIZE, 0);
    if(parse_status < 0) {
        printf("Couldn't send file completely!\n");
        exit(-1);
    }

    if(strcmp(buf, SUCCESS) == 0) {
        // send block by block
        // send the file block by block
        ssize_t read_ret;
        
        while(1) {
            bzero(buf, sizeof(buf));
            ssize_t read_ret = read(put_fd, buf + 3, BUFFER_SIZE-3);
            if(read_ret > 0) {
                buf[0] = 'M';
            } else if(read_ret == 0){
                buf[0] = 'L';                
            } else {
                perror("Error in reading from file!\n");
                exit(-1);
            }

            uint16_t temp = htons(read_ret);
            memcpy(buf + 1, &temp, sizeof(uint16_t));
            
            // printf("Sent(%ld): %c %s\n", read_ret+3, buf[0], buf + 3);
            int send_status = send(sockfd, buf, read_ret+3, 0);
            if(send_status < 0) {
                perror("Error in send\n");
                exit(-1);
            }
            if(read_ret == 0)
                break;
        }
        close(put_fd);
        return 0;
    } else if(strcmp(buf, ERROR1) == 0) {
        printf("Error 500, File doesnt exist or can't be opened for writing on server side!\n");
        return -1;
    }
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
    char buf[BUFFER_SIZE+1];
    int connected = 0;

    int state = START;
    while(state != QUIT) {
        printf("myFTP>");
        scanf(" %[^\n]", buf);
        // printf("Got : %s\n", buf);

        // check for quit
        if(strcmp(buf, "quit") == 0) {
            state = QUIT;
        } else if(strcmp(buf, "help") == 0) {
            printf("Command format\n");
            printf("1. open <ip> <port>\n");
            printf("2. user <username>\n");
            printf("3. pass <password>\n");
            printf("4. cd <dirname>\n");
            printf("5. lcd <dirname>\n");
            printf("6. dir\n");
            printf("7. get <remote_file> <local_file>\n");
            printf("8. put <local_file> <remote_file>\n");
            printf("9. mget <file1>,<file2>,<file3> ...\n");
            printf("10. mput <file1>,<file2>,<file3> ...\n");
            printf("11. quit\n");
            printf("12. ldir (extra feature)\n");
            printf("13. help (extra feature)\n");
            continue;
        }

        switch (state) {           
            case START: {
                // open ip port
                
                memset(port, '\0', sizeof(port));
                memset(ip, '\0', sizeof(ip));
                
                int ret = sscanf(buf,"open %s %s", ip, port);
                
                // printf("ret = %d\n", ret);
                // printf("Ip : %s\n", ip);
                // printf("Port : %s\n", port);

                if(ret < 2) {
                    // printf("len(buf) = %ld\n", strlen(buf));
                    printf("Incorrect command format!\n");
                    printf("Expected format is: open <ip> <port>\n");
                    printf("Type help for assistance with command formats\n");
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

                connected = 1;
                state = OPENED;
                break;
            }

            case OPENED: {
                // send username

                int send_status = send(sockfd, buf, strlen(buf) + 1, 0);
                if(send_status < 0) {
                    perror("Error in send\n");
                    exit(-1);
                }

                // printf("Sent to server\n");

                bzero(buf, sizeof(buf));
                int parse_status = recv(sockfd, buf, 4, 0);
                if(parse_status < 0) {
                    printf("Couldn't send file completely!\n");
                    exit(-1);
                }

                // printf("Received from server\n");

                if(strcmp(buf, SUCCESS) == 0) {
                    printf("Matched username\n");
                    state = GOT_USER;
                } else if (strcmp(buf, ERROR2) == 0) {
                    printf("Incorrect command sent!\n");
                    printf("Type help for assistance with command formats\n");
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
                int send_status = send(sockfd, buf, strlen(buf) + 1, 0);
                if(send_status < 0) {
                    perror("Error in send\n");
                    exit(-1);
                }

                bzero(buf, sizeof(buf));
                int parse_status = recv(sockfd, buf, 4, 0);
                if(parse_status < 0) {
                    printf("Couldn't send file completely!\n");
                    exit(-1);
                }

                if(strcmp(buf, SUCCESS) == 0) {
                    printf("Matched password\n");
                    state = AUTHENTICATED;
                } else if (strcmp(buf, ERROR2) == 0) {
                    printf("Incorrect command sent!\n");
                    printf("Type help for assistance with command formats\n");
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
                // extract the first word/ command from buffer
                char cmd[BUFFER_SIZE + 1];
                sscanf(buf, "%s", cmd);

                // commands 4-10 are executed here
                if(strcmp(cmd, "lcd")==0){
                    char path[BUFFER_SIZE + 1];
                    int ret = sscanf(buf,"lcd %s", path);
                    // printf("ret = %d, path = %s\n", ret, path);
                    if(ret < 1) {
                        printf("Incorrect command format!\n");
                        printf("Expected format is: lcd <dirname>\n");
                        break;
                    }
                    if(chdir(path)==-1) {
                        // error
                        perror("Couldn't change diretory\n");
                    }
                }
                else if(strcmp(cmd, "ldir")==0){
                    // Extra feature
                    DIR *dir;
                    struct dirent *dp;
                    char * file_name;
                    dir = opendir(".");
                    while ((dp=readdir(dir)) != NULL) {
                        if ( strcmp(dp->d_name, ".")==0 || strcmp(dp->d_name, "..")==0 )
                        {
                            // do nothing (straight logic)
                        } else
                            printf("%s\n", dp->d_name);
                    }
                    closedir(dir);
                }
                else if(strcmp(cmd, "get") == 0) {
                    receive_file(sockfd, buf);
                } 
                else if(strcmp(cmd, "put") == 0) {
                    send_file(sockfd, buf);
                }
                else if(strcmp(cmd, "mput") == 0){
                    // mput file1, file2, file3
                    char new_buf[BUFFER_SIZE + 1];
                    strcpy(new_buf, buf + 5);
                    // file1, file2, file3
                    char* filename = strtok(new_buf, ",");
 
                    // Keep printing filenames while one of the
                    while (filename != NULL) {
                        char temp[BUFFER_SIZE + 1];
                        bzero(temp, sizeof(temp));
                        sprintf(temp, "put %s %s", filename, filename);
                        // printf("%s\n", temp);
                        int status = send_file(sockfd, temp);
                        if(status < 0)
                            break;
                        filename = strtok(NULL, ",");
                    }
                }
                else if(strcmp(cmd, "mget") == 0){
                    // mget file1, file2, file3
                    char new_buf[BUFFER_SIZE + 1];
                    bzero(new_buf, sizeof(new_buf));
                    strcpy(new_buf, buf + 5);
                    // file1, file2, file3
                    char* filename = strtok(new_buf, ",");
 
                    // Keep printing filenames while one of the
                    while (filename != NULL) {
                        char temp[BUFFER_SIZE + 1];
                        bzero(temp, sizeof(temp));
                        sprintf(temp, "get %s %s", filename, filename);
                        // printf("%s\n", temp);
                        int status = receive_file(sockfd, temp);
                        if(status < 0)
                            break;
                        filename = strtok(NULL, ",");
                    }
                }

                else if(strcmp(cmd, "dir") == 0) {
                    int send_status = send(sockfd, buf, strlen(buf) + 1, 0);
                    if(send_status < 0) {
                        perror("Error in send\n");
                        exit(-1);
                    }

                    int flag = 1;
                    do {
                        bzero(buf, sizeof(buf));
                        int parse_status = recv(sockfd, buf, BUFFER_SIZE, 0);
                        if(parse_status < 0) {
                            perror("Error in recv!\n");
                            exit(-1);
                        }
                        if(parse_status == 0)
                        {
                            break;
                        }
                        if(buf[parse_status-1] == '\0' && buf[parse_status-2] == '\0')
                            flag = 0;
                            
                        char *b = buf;
                        while(strlen(b) > 0)
                        {
                            printf("%s\n", b);
                            b = b + strlen(b) + 1;
                        }
                        
                    } while(flag == 1);
                }
                else if(strcmp(cmd, "cd") == 0){
                    int send_status = send(sockfd, buf, strlen(buf) + 1, 0);
                    if(send_status < 0) {
                        perror("Error in send\n");
                        exit(-1);
                    }

                    bzero(buf, sizeof(buf));
                    int parse_status = recv(sockfd, buf, 4, 0);
                    if(parse_status < 0) {
                        perror("Error in recv!\n");
                        exit(-1);
                    }
                    if(strcmp(buf, SUCCESS) == 0) {
                        printf("Server dir changed successfully\n");
                    } else if (strcmp(buf, ERROR1) == 0) {
                        printf("Error 500, please try again!\n");
                    }
                } else {
                    printf("Invalid command entered!\n");
                    printf("Type help for assistance with command formats\n");
                }
                
                break;
            }
            case QUIT: {
                // send quit command to server!
                if(connected) {
                    int send_status = send(sockfd, buf, strlen(buf) + 1, 0);
                    if(send_status < 0) {
                        perror("Error in send\n");
                        exit(-1);
                    }
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
