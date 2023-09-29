#include <stdio.h>
#include "unp.h"
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char ** argv){
    int socket_FD= socket(AF_INET, SOCK_STREAM , 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9877 + atoi(argv[1]));
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if(bind(socket_FD, (struct sockaddr*)&server_addr, sizeof(server_addr))==-1){
        perror("Unable to bind socket");
        exit(EXIT_FAILURE);
    }
    listen(socket_FD, 1);
    struct sockaddr client_addr;
    unsigned int client_size = sizeof(client_addr);
    int client_sock = accept(socket_FD, (struct sockaddr*)&client_addr, &client_size);
    printf(">Accepted connection\n");
    char* buffer=calloc(1000, sizeof(char));
    while (scanf("%[^\n]%*c",buffer)!=EOF) {
        send(client_sock, buffer,strlen(buffer), 0);
        memset(buffer, 0, 1000);
    }
    close(client_sock);
    close(socket_FD);
    exit(EXIT_SUCCESS);
}

