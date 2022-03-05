#include "rsocket.h"

typedef struct _Message{
    int messageId;
    // dest ip and port 
    int sockfd;     // new
    char buf[BUFFER_SIZE + 1];
    size_t len;     // new
    int flags;      // new
    int retval;     // retval
    struct sockaddr addr;
    time_t sentTime;
} Message;

typedef struct _unackedMessageTable {
    pthread_mutex_t unackedTableMutex;
    Message unackedMessages[MAX_TABLE_SIZE];
    int size;
} unackedMessageTable;

typedef struct _readMessageTable {
    pthread_mutex_t readTableMutex;
    Message unreadMessages[MAX_TABLE_SIZE];
    int size;
} readMessageTable;

typedef struct _mrpSocket {
    readMessageTable myReadMessageTable;
    unackedMessageTable myUnackedMessageTable;
    pthread_t R_id, S_id;
    int sockfd;
    // struct sockaddr addr;
    // socklen_t addrlen;
} mrpSocket;

pthread_mutex_t global_lock;
mrpSocket* mySocket;
int ctr;

int isData(char* buf)
{
    // A utility function to check if the message is a data / ack message

    if(buf == NULL) {
        printf("Error Error in isData!");
        exit(-1);
    }

    if(buf[0] != 'A' && buf[0] != 'M') {
        printf("oh no no no\n");
    }
    printf("Check is Data %c\n", buf[0]);
    return (buf[0] == 'M') ? 1 : 0;
}

int extractMessageId(char *buf)
{
    // A utility function to extract message id from the buffer.

    if(buf == NULL) {
        printf("Error Error in extract!");
        exit(-1);
    }
    // buf[1:4] -> int
    int dest;

    memcpy(&dest, buf + 1, 4);

    int mid = ntohl(dest);
    return mid;
}

void copyMessage(Message* dest, Message* src) {
    // A utility function to copy messages
    if(src == NULL || dest == NULL) {
        printf("Nullptr exception in in copyMessage");
        exit(-1);
    }   
    // [sockfd, len, flags]
    dest->messageId = src->messageId;
    dest->sockfd = src->sockfd;
    dest->len = src->len;
    dest->flags = src->flags;
    dest->addr = src->addr;
    dest->sentTime = src->sentTime;
    dest->retval = src->retval;
    memcpy(dest->buf, src->buf, BUFFER_SIZE + 1);
}

int addmessage(char *buf, int retval, struct sockaddr *cli_addr) {
    // Add to read message table

    Message* newMessage = (Message *)malloc(sizeof(Message));
    newMessage->messageId = extractMessageId(buf);
    bzero(newMessage->buf, sizeof(newMessage->buf));
    strcpy(newMessage->buf, buf+5);
    printf("Message (%d) Copied %s to %s\n", newMessage->messageId, buf+5, newMessage->buf);
    newMessage->addr = *cli_addr;
    newMessage->retval = retval;

    pthread_mutex_lock(&(mySocket->myReadMessageTable.readTableMutex));
    // CHECK TODO
    

    for(int i = 0; i < MAX_TABLE_SIZE; i++) {
        Message* curMessage = &(mySocket->myReadMessageTable.unreadMessages[i]);
        if(curMessage->messageId == -1) {
            // add
            copyMessage(curMessage, newMessage);
            printf("Message added at i = %d, message = %s \n", i, curMessage->buf);
            mySocket->myReadMessageTable.size++;
            pthread_mutex_unlock(&(mySocket->myReadMessageTable.readTableMutex));
            return 1;
        }
    }
    pthread_mutex_unlock(&(mySocket->myReadMessageTable.readTableMutex));
    
    return 0;
}

void* routine_r(void* param) {
    // Wait on recvfrom periodically
    // If data message:
    //      add to read_msg_table
    // else:
    //      remove from unacked_msg_table
    
    mrpSocket* mySocket = (mrpSocket *)param;
    printf("Routine R called with sockfd : %d\n", mySocket->sockfd);
    struct sockaddr_in cli_addr;
    socklen_t addr_len;
    char buf[BUFFER_SIZE + 1];
    while(1)
    {
        bzero(buf, sizeof(buf));
        addr_len = sizeof(struct sockaddr);
        int retval;
        if((retval = recvfrom(mySocket->sockfd, buf, BUFFER_SIZE, 0, (struct sockaddr*)&cli_addr, &addr_len)) < 0)
        {
            perror("recvfrom()");
        }
        printf("Retvall(recv from) = %d\n", retval);
        // check if buf is data or ack
        printf("Routine R got %s %s\n",buf, buf+5);
        
        if(isData(buf))
        {
            // Add to read message table and send ack
            int mid = extractMessageId(buf);
            printf("Message recvd %d %s\n", mid, buf+5);
            // TODO
            // Message *newMessage = (Message *)  malloc(sizeof(Message));
            // initMwssage(newMessage);
            int ret = addmessage(buf, retval, (struct sockaddr*)&cli_addr);
            if(ret <= 0) {
                printf("Addmessage failed!\n");
                return NULL;
            }
            // ACK
            bzero(buf, sizeof(buf));
            buf[0]='A';
            printf("ACK SENT FOR MESSAGE %d, to %s:%d\n", mid, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            int src = htonl(mid);    // TODO: check
            memcpy(buf+1, &src, 4);
            if(sendto(mySocket->sockfd, buf, BUFFER_SIZE, 0, (struct sockaddr*)&cli_addr, addr_len) < 0)
            {
                perror("sendto()");
            }
        }
        else
        {
            int mid = extractMessageId(buf);
            printf("ACK RECVD for message %d from %s:%d\n", mid, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            // pop_from_ack_table
            // TODO

            pthread_mutex_lock(&(mySocket->myUnackedMessageTable.unackedTableMutex));
            for(int i = 0; i < MAX_TABLE_SIZE; i++) {
                Message* curMessage = &(mySocket->myUnackedMessageTable.unackedMessages[i]);
                
                if(curMessage->messageId == mid && memcmp(&curMessage->addr, &cli_addr, sizeof(struct sockaddr)) == 0 ) {
                    // TODO : add mutex lock for ctr
                    curMessage->messageId = -1;
                    mySocket->myUnackedMessageTable.size--;
                    // arrey ye gadbad hai kya?
                    // return sendto(sockfd, buf, len, flags, dest_addr, addrlen);
                }
            }
            pthread_mutex_unlock(&(mySocket->myUnackedMessageTable.unackedTableMutex));
        }
        
    }

    return NULL;   
}

void* routine_s(void* param) {
    // Thread S 
    
    // Check the unacked message table from time to time
    // Resend the timed out messages
    mrpSocket* mySocket = (mrpSocket *)param;

    while(1) {
        // sleep(rand()%MAX_SLEEP); //TODO
        // printf("Routine S in force\n");

        usleep(500000);
        for(int i = 0; i < MAX_TABLE_SIZE; i++) {
            // TODO Check
            pthread_mutex_lock(&(mySocket->myUnackedMessageTable.unackedTableMutex));
            Message *cur = &(mySocket->myUnackedMessageTable.unackedMessages[i]); 
            if(cur->messageId == -1 || (time(NULL) - cur->sentTime) <= THRESHOLD_TIME) {
                pthread_mutex_unlock(&(mySocket->myUnackedMessageTable.unackedTableMutex));
                continue;
            }
            // resend message from here and update the sentTime
            printf("Message Here %d %s\n",cur->messageId ,  cur->buf + 5);
            mySocket->myUnackedMessageTable.unackedMessages[i].sentTime = time(NULL);
            socklen_t addrlen = sizeof(cur->addr);
            sendto(mySocket->sockfd, cur->buf, BUFFER_SIZE, 0, (const struct sockaddr*)(&(cur->addr)), addrlen);
            struct sockaddr_in *tempaddr = (struct sockaddr_in *)&(cur->addr);
            printf("Unack Message %s sent to %s:%d\n", cur->buf, inet_ntoa(tempaddr->sin_addr), ntohs(tempaddr->sin_port));
            pthread_mutex_unlock(&(mySocket->myUnackedMessageTable.unackedTableMutex));
        }
    }
    return NULL;
}

void initMessage(Message* myMessage) {
    // Utility function to initialize a message
    myMessage->messageId = -1;
    memset(myMessage->buf, '\0', sizeof(myMessage->buf));
}

void initSocket(mrpSocket* mySocket) {
    // Utility function to initialize data structures and the mutex locks
    mySocket->myReadMessageTable.size = mySocket->myUnackedMessageTable.size = 0;
    for(int i = 0; i < MAX_TABLE_SIZE; i++) {
        initMessage(&(mySocket->myReadMessageTable.unreadMessages[i]));
        initMessage(&(mySocket->myUnackedMessageTable.unackedMessages[i]));
    }

    // 2 Mutex for tables
    pthread_mutex_init(&(mySocket->myUnackedMessageTable.unackedTableMutex), NULL);
    pthread_mutex_init(&(mySocket->myReadMessageTable.readTableMutex), NULL);

    // 1 global mutex
    pthread_mutex_init(&global_lock, NULL);
}

int r_socket(int domain, int type, int protocol) {
    // Create the data structures : 2 tables 
    // Create 2 threads : S and R

    if(type != SOCK_MRP)
        return ERROR;

    int ret = socket(domain, SOCK_DGRAM, protocol);
    if(ret < 0)
    {
        perror("Socket()");
        return ret;
    }
    ctr = 0;
    // Init data structures
    mySocket = (mrpSocket *)malloc(sizeof(mrpSocket));
    initSocket(mySocket);
    mySocket->sockfd = ret;

    // Create 2 threads
    pthread_create(&(mySocket->R_id), NULL, routine_r, mySocket);
    pthread_create(&(mySocket->S_id), NULL, routine_s, mySocket);
    
    return ret;
}

int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    // Just an ordinary bind
    int ret = bind(sockfd, addr, addrlen);
    if(ret < 0)
    {
        perror("bind()");
    }
    return ret;
}

ssize_t r_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
    printf("Rsendto called\n");
    // put msg in after sending
    // TODO : maybe send first and then carry out the functionalities?

    // Design the message
    Message* newMessage = (Message *)malloc(sizeof(Message));
    newMessage->buf[0] = 'M';
    newMessage->messageId = ctr;
    int src = htonl(ctr);
    memcpy(newMessage->buf + 1, &src, 4);
    memcpy(newMessage->buf + 5, (const char*)buf, len);
    printf("[R_SENDTO] %s %s\n" , (char*)buf, newMessage->buf + 5);
    newMessage->addr = *dest_addr;
    struct sockaddr_in *tempaddr = (struct sockaddr_in *)&(newMessage->addr);
    printf("Addr copied to %s:%d\n", inet_ntoa(tempaddr->sin_addr), ntohs(tempaddr->sin_port));
    // mySocket --> table

    
    int retVal = sendto(sockfd, newMessage->buf, BUFFER_SIZE, flags, dest_addr, addrlen);
    if(retVal < 0)
        return retVal;

    int flag = 0;
    pthread_mutex_lock(&(mySocket->myUnackedMessageTable.unackedTableMutex));
    for(int i = 0; i < MAX_TABLE_SIZE; i++) {
        Message* curMessage = &(mySocket->myUnackedMessageTable.unackedMessages[i]);
        if(curMessage->messageId == -1) {
            printf("Sent this %s and ctr was %d\n", newMessage->buf + 5, ctr);
            // add
            copyMessage(curMessage, newMessage);
            // TODO : add mutex lock for ctr
            curMessage->messageId = ctr;
            printf("A new message %d added\n", ctr);
            ctr++;
            flag = 1;
            break;
        }
    }
    pthread_mutex_unlock(&(mySocket->myUnackedMessageTable.unackedTableMutex));
    if(!flag)
    {
        printf("No space Available in unacked tablee\n");
    }
    return flag ? retVal:ERROR;
}

ssize_t r_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    
    // while true :
    //     if any message in read_msg_table
    //         pop it
    //         return 1st message
    //     else
    //         sleep
    

    while(1) {
        if(mySocket->myReadMessageTable.size) {
            pthread_mutex_lock(&(mySocket->myReadMessageTable.readTableMutex));
            for(int i = 0; i < MAX_TABLE_SIZE; i++) {
                Message* curMessage = &(mySocket->myReadMessageTable.unreadMessages[i]);
                if(curMessage->messageId != -1) {
                    // add
                    memcpy(buf, curMessage->buf, len);
                    *src_addr = curMessage->addr; 
                    // TODO : add mutex lock for ctr
                    curMessage->messageId = -1;
                    mySocket->myReadMessageTable.size--;
                    pthread_mutex_unlock(&(mySocket->myReadMessageTable.readTableMutex));
                    
                    return curMessage->retval;
                }
            }
            pthread_mutex_unlock(&(mySocket->myReadMessageTable.readTableMutex));
            
        } else {
            sleep(RECVFROM_SLEEP_TIME);
        }
    }

    return ERROR;
}

int r_close(int fd) {
    
    if(fd != mySocket->sockfd)
        return -1;

    int res = close(fd);
    if(res < 0)
        return res;

    // free the tables
    free(mySocket);     // rest of the items are static in nature.. they will be destroyed automatically!

    // terminate S and R
    // close()
    pthread_kill(mySocket->R_id, SIGINT);
    pthread_kill(mySocket->S_id, SIGINT);

    pthread_join(mySocket->R_id, NULL);
    pthread_join(mySocket->S_id, NULL);

    pthread_mutex_destroy(&(mySocket->myReadMessageTable.readTableMutex));
    pthread_mutex_destroy(&(mySocket->myUnackedMessageTable.unackedTableMutex));
    
    return res;
}