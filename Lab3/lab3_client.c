#include <netinet/in.h>
#include "unp.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define PORT 9876
struct sockaddr_in server_addr;
int get_socket(){
    int socket_FD= socket(AF_INET, SOCK_STREAM , 0);
    server_addr.sin_port = htons(0);
    if(bind(socket_FD, (struct sockaddr*)&server_addr, sizeof(server_addr))==-1){
        perror("Unable to bind socket");
        exit(EXIT_FAILURE);
    }
    return socket_FD;
}

int main(int argc, char ** argv){
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    int num_connect=0;
    fd_set readfd, ready;
    FD_ZERO(&readfd);
    FD_SET(STDIN_FILENO, &readfd);
    while(1){
        ready= readfd;
        select(FD_SETSIZE, &ready, NULL, NULL, NULL);
        for(int i=0; i< FD_SETSIZE; i++){
            if(FD_ISSET(i, &ready)){
                if(i==STDIN_FILENO){
                    int newport=0;
                    scanf("%d",&newport);
                    if(num_connect<5){
                        int socket_FD= get_socket();
                        server_addr.sin_port = htons(newport);
                        if(connect(socket_FD, (struct sockaddr*)&server_addr, sizeof(server_addr))){
                            perror("Unable to connect\n");
                            exit(EXIT_FAILURE);
                        }
                        FD_SET(socket_FD, &readfd);
                    }

                }
                else{
                    char message[512];
                    memset(message, 0, 512);
                    int amount=read(i, message, 512);
                    struct sockaddr_in server_addr2;
                    socklen_t len;
                    getpeername(i, (struct sockaddr*)&server_addr2, &len);
                    if(amount==0){
                        printf(">Server on %d closed\n",ntohs(server_addr2.sin_port));
                        FD_CLR(i, &readfd);
                        close(i);
                    }
                    else{
                        printf(">%d %s\n",ntohs(server_addr.sin_port),message);
                    }
                }
            }
        }
    }
}
