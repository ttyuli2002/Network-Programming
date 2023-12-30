#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>

int main(int argc, char ** argv){
    int socket_FD= socket(AF_INET, SOCK_STREAM , 0);
    if(socket_FD<0){
        perror("Invalid Socket");
        exit(EXIT_FAILURE);
    }
    int optval=0;
    unsigned int size=sizeof(int);
    getsockopt(socket_FD, IPPROTO_TCP, TCP_MAXSEG, &optval, &size);
    printf("TCP max seggment size before connecting %d\n",optval);
    getsockopt(socket_FD, IPPROTO_TCP, SO_RCVBUF, &optval, &size);
    printf("TCP recieve buffer size before connecting %d\n",optval);
    struct sockaddr_in client_addr;
    client_addr.sin_port = htons(80);
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(argv[1]);
    if(connect(socket_FD, (struct sockaddr*)&client_addr, sizeof(client_addr))){
        perror("Cant connect");
        exit(EXIT_FAILURE);
    }
    getsockopt(socket_FD, IPPROTO_TCP, TCP_MAXSEG, &optval, &size);
    printf("TCP max seggment size before connecting %d\n",optval);
    getsockopt(socket_FD, IPPROTO_TCP, SO_RCVBUF, &optval, &size);
    printf("TCP recieve buffer size before connecting %d\n",optval);
    close(socket_FD);
}
