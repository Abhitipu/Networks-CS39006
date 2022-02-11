/**THE TCP SERVER**/
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h>
#include <fcntl.h>
#include <dirent.h>

#include <sys/stat.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <netdb.h>

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

#define BUFFER_SIZE 200
#define PORT 20001

int Min(int a, int b) {
    return (a < b) ? a : b;
}

// Appends buf2 string at buf+bufsize, and return new len of strings (including 1 null at the end)
int mycat(char *buf, int bufsize, char *buf2)
{
    strcat(buf+bufsize, buf2);
    return bufsize + strlen(buf2) + 1;
}

// Check if the user exists in the input file or not
int user_exists(char* username)
{
    FILE *fp;
    int c;
    fp = fopen("user.txt","r");
    if (fp == NULL) {
		perror("fopen: ");
		return 0;
	}
    char buf1[100] , buf2[100];
    while(fscanf(fp, "%s %s", buf1, buf2) > 0)
    {
        // usernames
        if(strcmp(buf1, username) == 0)
        {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;

}

// Check if the user exists in the input file or not
int password_exists(char* username, char* password) {
    FILE *fp;
    int c;
    fp = fopen("user.txt","r");
    if (fp == NULL) {
		perror("fopen: ");
		return 0;
	}
    char buf1[100] , buf2[100];
    while(fscanf(fp, "%s %s", buf1, buf2) > 0)
    {
        // usernames
        if((strcmp(buf1, username) == 0) && strcmp(buf2, password) == 0)
        {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

// the wrapper for sending blocks of text from the server to client
void send_file(int tcp_newsockfd, char* buf) {
    // try to open remote_file
    char remote_file[BUFFER_SIZE + 1], local_file[BUFFER_SIZE + 1];
    sscanf(buf, "get %s %s", remote_file, local_file);
    
    // check if server can open file
    int get_fd = open(remote_file, O_RDONLY);
    if(get_fd < 0) {
        printf("Can't open file from server side!\n");
        bzero(buf, sizeof(buf));
        strcpy(buf, ERROR1);
        // Sending 500 if it cant
        int send_status = send(tcp_newsockfd, buf, strlen(buf) + 1, 0);
        if(send_status < 0) {
            perror("Error in send\n");
            exit(-1);
        }
        return;
    }
    bzero(buf, sizeof(buf));
    strcpy(buf, SUCCESS);
    int send_status = send(tcp_newsockfd, buf, strlen(buf) + 1, 0);
    if(send_status < 0) {
        perror("Error in send\n");
        exit(-1);
    }

    // Sending the file once it ensures that the server can read from it
    while(1) {
        bzero(buf, sizeof(buf));
        int read_ret = read(get_fd, buf + 3, BUFFER_SIZE-3);
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

        int send_status = send(tcp_newsockfd, buf, read_ret + 3, 0);
        // printf("Sending(%d) %s\n", read_ret, buf + 3);
        if(send_status < 0) {
            perror("Error in send\n");
            exit(-1);
        }
        if(read_ret == 0)
            break;
    }
    close(get_fd);
    return;
}

// the wrapper for sending blocks of text from the server to client
void receive_file(int sockfd, char* buf) {
    char remote_file[BUFFER_SIZE + 1], local_file[BUFFER_SIZE + 1];
    sscanf(buf, "put %s %s", local_file, remote_file);
    
    // check if the server can open the file
    int put_fd = open(remote_file, O_CREAT | O_TRUNC | O_WRONLY);
    if(put_fd < 0) {
        printf("Can't open file from server side!\n");
        bzero(buf, sizeof(buf));
        strcpy(buf, ERROR1);
        int send_status = send(sockfd, buf, strlen(buf) + 1, 0);
        if(send_status < 0) {
            perror("Error in send\n");
            exit(-1);
        }
        return;
    }
    bzero(buf, sizeof(buf));
    strcpy(buf, SUCCESS);
    int send_status = send(sockfd, buf, strlen(buf) + 1, 0);
    if(send_status < 0) {
        perror("Error in send\n");
        exit(-1);
    }

    // start file receive
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

        // Receiving the data block by block         
        for(uint16_t cur = 0; cur < len; cur += parse_status) {
            bzero(buf, sizeof(buf));
            parse_status = recv(sockfd, buf, Min(BUFFER_SIZE, len - cur), 0);
            if(parse_status < 0) {
                perror("Error in recv!\n");
                exit(-1);
            }
            if(write(put_fd, buf, parse_status) < 0)
            {
                perror("can't write");
                exit(1);
            }
        }

        if(flag == 'L')
        {
            break;
        }        
    }
    close(put_fd);
    return;
}

int main(int argc, char* argv[]) { 
	int	tcp_sockfd, tcp_newsockfd;
	struct sockaddr_in	tcp_cliaddr, tcp_servaddr;

    // Create tcp socket
	if ((tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}

    // Reset variables
    memset(&tcp_cliaddr, 0, sizeof(tcp_cliaddr));
    memset(&tcp_servaddr, 0, sizeof(tcp_servaddr));
      
    // All this is server info
	tcp_servaddr.sin_family		= AF_INET;
	tcp_servaddr.sin_addr.s_addr	= INADDR_ANY;   
	tcp_servaddr.sin_port		= htons(PORT);     // Network Byte Order or big endian system

    // Bind the tcp socket with the server address 
	if (bind(tcp_sockfd, (struct sockaddr *) &tcp_servaddr, sizeof(tcp_servaddr)) < 0) {
		perror("Unable to bind local address\n");
		exit(0);
	}

	listen(tcp_sockfd, 5);  //  5 concurrent connections for tcp

    printf("Server started\n");

	socklen_t tcp_clilen = sizeof(tcp_cliaddr);

    char buf[BUFFER_SIZE + 1], buf2[BUFFER_SIZE + 1]; 
    char username[BUFFER_SIZE + 1], password[BUFFER_SIZE + 1];

    memset(buf2, '\0', sizeof(buf2));
    memset(buf, '\0', sizeof(buf));
    memset(username, '\0', sizeof(username));
    memset(password, '\0', sizeof(password));
    
    int state = START;
    while(state == START)
    {
        // TCP
        // Maintaining file descriptors
        fd_set myfd;
        FD_ZERO(&myfd);
        FD_SET(tcp_sockfd, &myfd);

        // Server waits for a considerably long time for the initial connection
        struct timeval timer;
        timer.tv_sec = 300;
        timer.tv_usec = 0;

        int select_status = select(tcp_sockfd + 1, &myfd, 0, 0, NULL);

        if(select_status < 0) {
            perror("Error in select\n");
            exit(-1);
        }

        if(FD_ISSET(tcp_sockfd, &myfd)) {
            // Handle the tcp connection with a fork (concurrent)
            tcp_newsockfd = accept(tcp_sockfd, (struct sockaddr *) &tcp_cliaddr, &tcp_clilen);
            printf("TCP Connection Requested from %s:%d\n",inet_ntoa(tcp_cliaddr.sin_addr), ntohs(tcp_cliaddr.sin_port));
            if (tcp_newsockfd < 0) {
                perror("Accept error\n");
                exit(0);
            }

            if(fork() == 0) {
                // child: Handles the new tcp connection
                // close(tcp_sockfd);
                state = OPENED;
                break;
            } else {
                // parent
                
            }
        }

        if(FD_ISSET(tcp_sockfd, &myfd) == 0) {
            // timed out
            printf("Server timed out\n");
            state = QUIT;
            close(tcp_sockfd);
            exit(1);
        }
    }

    while(state != QUIT) {
        // common recv
        bzero(buf, sizeof(buf));
        int recv_status = recv(tcp_newsockfd, buf, BUFFER_SIZE, 0);
        if(recv_status < 0) {
            perror("Error in recv!\n");
            exit(-1);
        }
        if(strcmp(buf, "quit") == 0) {
            state = QUIT;
        }

        switch (state) {           

            case OPENED: {
                printf("In opened\n");
                
                int ret = sscanf(buf,"user %s", username);
                bzero(buf, sizeof(buf));

                if(ret < 1) {
                    printf("Incorrect command format!\n");
                    printf("Expected format is: user <username>\n");
                    // incorrect command : return 600
                    strcpy(buf, ERROR2);
                }
                else
                {
                    // check for user in user.txt
                    if(user_exists(username)) {
                        printf("%s accepted\n", username);
                        // correct username : return 200
                        strcpy(buf, SUCCESS);
                        state = GOT_USER;
                    } else {
                        // incorrect username : return 500
                        strcpy(buf, ERROR1);
                    }
                }
                int send_status = send(tcp_newsockfd, buf, strlen(buf) + 1, 0);
                if(send_status < 0) {
                    perror("Error in send\n");
                    exit(-1);
                }
                
                break;
            }
            case GOT_USER: {
                // Get password corresponding to the user
                int ret = sscanf(buf,"pass %s", password);
                bzero(buf, sizeof(buf));
                if(ret < 1) {
                    printf("Incorrect command format!\n");
                    printf("Expected format is: pass <password>\n");

                    // incorrect command : return 600
                    strcpy(buf, ERROR2);
                    state = OPENED;
                }
                else
                {
                    // check for corresponding password in user.txt
                    if(password_exists(username, password)) {
                        printf("%s accepted\n", password);
                        // incorrect username : return 500
                        strcpy(buf, SUCCESS);
                        state = AUTHENTICATED;
                    } else {
                        // incorrect password : return 500
                        strcpy(buf, ERROR1);
                        state = OPENED;
                    }
                }

                int send_status = send(tcp_newsockfd, buf, strlen(buf) + 1, 0);
                if(send_status < 0) {
                    perror("Error in send\n");
                    exit(-1);
                }

                break;
            }
            case AUTHENTICATED: {
                // extract the first word/ command from buffer
                char cmd[BUFFER_SIZE + 1];
                sscanf(buf, "%s", cmd);
                // printf("The command received is %s\n", cmd);
                if(strcmp(cmd, "dir") == 0) {
                    DIR *dir;
                    struct dirent *dp;
                    char * file_name;
                    dir = opendir(".");
                    bzero(buf, sizeof(buf));
                    int cur = 0;
                    while ((dp=readdir(dir)) != NULL) {
                        if ( strcmp(dp->d_name, ".")==0 || strcmp(dp->d_name, "..")==0 )
                        {
                            // do nothing (straight logic)
                        } else 
                        {
                            int check = strlen(dp->d_name);
                            // check the length
                            if(cur + check + 2 < BUFFER_SIZE) {
                                cur = mycat(buf, cur, dp->d_name);
                            } else {
                                // send
                                int dir_len = 0;
                                char *b = buf;
                                while(strlen(b) > 0)
                                {
                                    // printf("%s\n", b);
                                    dir_len+= strlen(b) + 1;
                                    b = b + strlen(b) + 1;
                                }
                                // bzero(buf, sizeof(buf));
                                // sending empty string at the end
                                // need to calculate actual size
                                int send_status = send(tcp_newsockfd, buf, dir_len, 0);
                                if(send_status < 0) {
                                    perror("Error in send\n");
                                    exit(-1);
                                }
                                bzero(buf, sizeof(buf));
                                cur = 0;
                                cur = mycat(buf, cur, dp->d_name);
                            }
                        }
                    }
                    
                    int send_status = send(tcp_newsockfd, buf, cur + 1, 0);
                    if(send_status < 0) {
                        perror("Error in send\n");
                        exit(-1);
                    }
                    closedir(dir);

                } else if(strcmp(cmd, "cd") == 0){
                    char path[BUFFER_SIZE + 1];
                    int ret = sscanf(buf,"cd %s", path);
                    bzero(buf, sizeof(buf));
                    if(ret < 1) {
                        printf("Incorrect command format!\n");
                        printf("Expected format is: cd <dirname>\n");
                        // incorrect command : return 500
                        strcpy(buf, ERROR1);
                    }
                    else{
                        if(chdir(path)==-1) {
                            // error
                            strcpy(buf, ERROR1);
                            perror("couldn\'t change diretory\n");
                        }
                        else{
                            strcpy(buf, SUCCESS);
                        }
                    }
                    int send_status = send(tcp_newsockfd, buf, strlen(buf) + 1, 0);
                    if(send_status < 0) {
                        perror("Error in send\n");
                        exit(-1);
                    }

                }
                else if(strcmp(cmd, "get") == 0){
                    send_file(tcp_newsockfd, buf);
                }
                else if(strcmp(cmd, "put") == 0){
                    receive_file(tcp_newsockfd, buf);
                }
                break;
            }
            case QUIT: {
                // Now we will close it
                printf("TCP Connection from %s:%d is closed!\n",inet_ntoa(tcp_cliaddr.sin_addr), ntohs(tcp_cliaddr.sin_port));
                close(tcp_sockfd);
                close(tcp_newsockfd);
                break;
            }
            default:
                break;
        }
    }

    return 0; 
}
