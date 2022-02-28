#include <rsocket.h>

int r_socket(int domain, int type, int protocol) {
    /*
        Create the data structures : 2 tables 
        Create 2 threads : S and R
        socket(, SOCK_STREAM, )
    */
    return ERROR;
}

int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return bind(sockfd, addr, addrlen);
}

ssize_t r_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
    /*
        sendto()
        put msg in unacked_msg_table
    */
    return ERROR;
}

ssize_t r_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    /*
        while true :
        if any message in read_msg_table
            pop it
            return 1st message
        else
            sleep
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