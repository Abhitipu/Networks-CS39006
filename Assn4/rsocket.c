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
} mrpSocket;

pthread_mutex_t global_lock;
mrpSocket* mySocket;
int ctr;
int nTransmissions, nMessages;


float getRandomFloat() {
    return (float)rand() / (float)RAND_MAX;
}

int dropMessage(float p) {
    float genrandom = getRandomFloat();
    if(genrandom < p)
    {
        return 1;
    }
    return 0;
}

int isData(char* buf)
{
    // A utility function to check if the message is a data / ack message

    if(buf == NULL) {
        printf("Error in isData!");
        exit(-1);
    }

    return (buf[0] == 'M') ? 1 : 0;
}

void handleExit(int signum) {
    if(signum == SIGINT) {
        float dropProb = DROP_PROBABILITY;
        printf("%.2f %d %d %.2f\n", dropProb, nMessages, nTransmissions, (float)nTransmissions / (float)nMessages);
    }
    exit(signum);
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
    newMessage->addr = *cli_addr;
    newMessage->retval = retval;

    pthread_mutex_lock(&(mySocket->myReadMessageTable.readTableMutex));

    for(int i = 0; i < MAX_TABLE_SIZE; i++) {
        Message* curMessage = &(mySocket->myReadMessageTable.unreadMessages[i]);
        if(curMessage->messageId == -1) {
            // add
            copyMessage(curMessage, newMessage);
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
        // check if buf is data or ack
        
        if(dropMessage(DROP_PROBABILITY)) {
            continue;
        }
        
        if(isData(buf))
        {
            // Add to read message table and send ack
            int mid = extractMessageId(buf);

            int ret = addmessage(buf, retval, (struct sockaddr*)&cli_addr);
            if(ret <= 0) {
                printf("Addmessage failed!\n");
                return NULL;
            }
            // ACK
            bzero(buf, sizeof(buf));
            buf[0]='A';
            printf("ACK SENT FOR MESSAGE %d, to %s:%d\n", mid, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            int src = htonl(mid);    
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

            pthread_mutex_lock(&(mySocket->myUnackedMessageTable.unackedTableMutex));
            for(int i = 0; i < MAX_TABLE_SIZE; i++) {
                Message* curMessage = &(mySocket->myUnackedMessageTable.unackedMessages[i]);
                
                if(curMessage->messageId == mid && memcmp(&curMessage->addr, &cli_addr, sizeof(struct sockaddr)) == 0 ) {
                    curMessage->messageId = -1;
                    mySocket->myUnackedMessageTable.size--;
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

        sleep(T);
        for(int i = 0; i < MAX_TABLE_SIZE; i++) {
            pthread_mutex_lock(&(mySocket->myUnackedMessageTable.unackedTableMutex));
            Message *cur = &(mySocket->myUnackedMessageTable.unackedMessages[i]); 
            if(cur->messageId == -1 || (time(NULL) - cur->sentTime) <= 2 * T) {
                pthread_mutex_unlock(&(mySocket->myUnackedMessageTable.unackedTableMutex));
                continue;
            }
            // resend message from here and update the sentTime
            mySocket->myUnackedMessageTable.unackedMessages[i].sentTime = time(NULL);
            socklen_t addrlen = sizeof(cur->addr);
            sendto(mySocket->sockfd, cur->buf, BUFFER_SIZE, 0, (const struct sockaddr*)(&(cur->addr)), addrlen);
            struct sockaddr_in *tempaddr = (struct sockaddr_in *)&(cur->addr);
            printf("Unack Message %s sent to %s:%d\n", cur->buf, inet_ntoa(tempaddr->sin_addr), ntohs(tempaddr->sin_port));
            nTransmissions++;
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

    signal(SIGINT, handleExit);

    nTransmissions = 0;
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
    // put msg in after sending

    // Design the message
    Message* newMessage = (Message *)malloc(sizeof(Message));
    newMessage->buf[0] = 'M';
    newMessage->messageId = ctr;
    int src = htonl(ctr);
    memcpy(newMessage->buf + 1, &src, 4);
    memcpy(newMessage->buf + 5, (const char*)buf, len);
    newMessage->addr = *dest_addr;
    // mySocket --> table

    int retVal = sendto(sockfd, newMessage->buf, BUFFER_SIZE, flags, dest_addr, addrlen);
    if(retVal < 0)
        return retVal;

    int flag = 0;
    pthread_mutex_lock(&(mySocket->myUnackedMessageTable.unackedTableMutex));
    for(int i = 0; i < MAX_TABLE_SIZE; i++) {
        Message* curMessage = &(mySocket->myUnackedMessageTable.unackedMessages[i]);
        if(curMessage->messageId == -1) {
            // add
            copyMessage(curMessage, newMessage);
            curMessage->messageId = ctr;
            curMessage->sentTime = time(NULL);
            pthread_mutex_lock(&global_lock);
            ctr++;
            pthread_mutex_unlock(&global_lock);
            nTransmissions++;
            nMessages++;
            flag = 1;
            mySocket->myUnackedMessageTable.size++;
            break;
        }
    }
    pthread_mutex_unlock(&(mySocket->myUnackedMessageTable.unackedTableMutex));
    if(!flag)
    {
        printf("No space Available in unacked table\n");
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

// abcdefghijklmnopqrstuvwxyz1234567890
int r_close(int fd) {
    
    if(fd != mySocket->sockfd)
        return -1;
    
    // terminate S and R
    pthread_kill(mySocket->R_id, SIGINT);
    pthread_kill(mySocket->S_id, SIGINT);

    pthread_join(mySocket->R_id, NULL);
    pthread_join(mySocket->S_id, NULL);

    int res = close(fd);
    if(res < 0)
        return res;

    pthread_mutex_destroy(&(mySocket->myReadMessageTable.readTableMutex));
    pthread_mutex_destroy(&(mySocket->myUnackedMessageTable.unackedTableMutex));
    
    // free the tables
    free(mySocket);     // rest of the items are static in nature.. they will be destroyed automatically!
    
    return res;
}