#include <rsocket.h>

typedef struct _Message{
    int messageId;
    // dest ip and port
    struct sockaddr_in addr;
    char buf[BUFFER_SIZE];
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

int ctr;
int isData(char* buf)
{
    if(buf == NULL) {
        printf("Error Error in isData!");
        exit(-1);
    }
    return (buf[0] == 'M') ? 1 : 0;
}
int extractMessageId(char *buf)
{
    if(buf == NULL) {
        printf("Error Error in extract!");
        exit(-1);
    }
    // {buf[1], buf[2], buf[3], buf[4]} -> int
    int dest;

    memcpy(&dest, buf + 1, 4);

    int mid = ntohl(dest);
    return mid;
}
int addmessage(char *buf, mrpSocket* mySocket) {
    // Add to table
    // M
    // message.buf
    // message.addr -> ip + port
    // 
    // M/A 4byte messageId rand int -> message sequence dest_address-port(4+2 bytes) 
    // It also adds a message sequence
    // no. at the beginning of the message and stores the message along with its sequence no.
    // and destination address-port in the unacknowledged-message table before sending the
    // message. With each entry, there is also a time field that is filled up initially with the
    // time of first sending the message.
    pthread_mutex_lock(&(mySocket->myReadMessageTable.readTableMutex));
    // CHECK TODO
    int index = mySocket->myReadMessageTable.size;
    memcpy(mySocket->myReadMessageTable.unreadMessages[index].buf, buf, sizeof(buf));
    mySocket->myReadMessageTable.size++;
    pthread_mutex_unlock(&(mySocket->myReadMessageTable.readTableMutex));

    
    return 0;
}
void* routine_r(void* param) {
    mrpSocket* mySocket = (mrpSocket *)param;
    struct sockaddr_in cli_addr;
    int addr_len;
    char buf[BUFFER_SIZE];
    while(1)
    {
        bzero(buf, sizeof(buf));
        addr_len = sizeof(struct sockaddr);
        if( recvfrom(mySocket->sockfd, buf, BUFFER_SIZE-1, 0, (struct sockaddr*)&cli_addr, &addr_len) < 0)
        {
            perror("recvfrom()");
        }
        // check if buf is data or ack
        
        if(isData(buf))
        {
            // Add to message table
            printf("Message recvd %s\n");
            // TODO
            // Message *newMessage = (Message *)  malloc(sizeof(Message));
            // initMwssage(newMessage);
            iaddmessage(buf);
            // ACK
            bzero(buf, sizeof(buf));
            buf[0]='A';
            int src = extractMessageId(buf);    // TODO: check
            memcpy(buf+1, &src, 4);
            if(sendto(mySocket->sockfd, buf, BUFFER_SIZE-1, 0, (struct sockaddr*)&cli_addr, &addr_len) < 0)
            {
                perror("sendto()");
            }
        }
        else
        {
            int mid = extractMessageId(buf);
            // pop_from_ack_table
            // TODO
        }
        
    }

    return NULL;   
}

void* routine_s(void* param) {
    mrpSocket* mySocket = (mrpSocket *)param;

    while(1) {
        sleep(rand()%MAX_SLEEP);
        for(int i = 0; i < MAX_TABLE_SIZE; i++) {
            // TODO Check
            Message cur = mySocket->myUnackedMessageTable.unackedMessages[i]; 
            if(cur.messageId == -1 || (time(NULL) - cur.sentTime) <= THRESHOLD_TIME)
                continue;
            // resend message from here and update the sentTime
            mySocket->myUnackedMessageTable.unackedMessages[i].sentTime = time(NULL);
            size_t addrlen = sizeof(cur.addr);
            sendto(mySocket->sockfd, cur.buf, strlen(cur.buf), 0, (const struct sockaddr*)(&(cur.addr)), &addrlen);
        }
    }
    return NULL;
}

void initMessage(Message* myMessage) {
    myMessage->messageId = -1;
    memset(myMessage->buf, '\0', sizeof(myMessage->buf));
}

void initSocket(mrpSocket* mySocket) {
    mySocket->myReadMessageTable.size = mySocket->myUnackedMessageTable.size = 0;
    for(int i = 0; i < MAX_TABLE_SIZE; i++) {
        initMessage(&(mySocket->myReadMessageTable.unreadMessages[i]));
        initMessage(&(mySocket->myUnackedMessageTable.unackedMessages[i]));
    }
    // 2 Mutex for tables
    pthread_mutex_init(&(mySocket->myUnackedMessageTable.unackedTableMutex), NULL);
    pthread_mutex_init(&(mySocket->myReadMessageTable.readTableMutex), NULL);
}

int r_socket(int domain, int type, int protocol) {
    // Create the data structures : 2 tables 
    // Create 2 threads : S and R

    if(type != SOCK_MRP)
        return ERROR;
    ctr = 0;
    // Init data structures
    mrpSocket* mySocket = (mrpSocket *)malloc(sizeof(mrpSocket));
    initSocket(mySocket);

    // Create 2 threads
    pthread_create(&(mySocket->R_id), NULL, routine_r, mySocket);
    pthread_create(&(mySocket->S_id), NULL, routine_s, mySocket);
    int ret = socket(domain, SOCK_DGRAM, protocol);
    mySocket->sockfd = ret;
    return ret;
}

int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return bind(sockfd, addr, addrlen);
}

ssize_t r_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
    /*
        sendto()
        put msg in unacked_msg_table
    */
    
    Message* newMessage = (Message *)malloc(sizeof(Message));
    newMessage->buf[0] = 'M';
    newMessage->messageId = ctr;
    int src = htonl(ctr);
    memcpy(newMessage->buf + 1, src, 4);
    memcpy(newMessage->buf + 5, (const char*)buf, BUFFER_SIZE - 5);
    
    // for(int i = 0; i < MAX_TABLE_SIZE; i++) {
    //     if()
    // }

    ctr++;
    return ERROR;
}

ssize_t r_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    /*
        // while true :
        //     if any message in read_msg_table
        //         pop it
        //         return 1st message
        //     else
        //         sleep
        
        if message is dataMessage:
            put in read_msg_table and send ack
        else:
            remove from unacked_msg_table
    */
    return ERROR;
}

int r_close(int fd) {
    /*
        free the tables
        terminate S and R
        close()
    */
    return ERROR;
}

// int main() {

//     return 0;
// }