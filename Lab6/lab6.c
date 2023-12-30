#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

int main(int argc, char ** argv){
    struct addrinfo * res= NULL;
    struct addrinfo hints;
    memset(&hints, 0,sizeof(hints));
    hints.ai_flags = AI_ALL;
    hints.ai_socktype = SOCK_RAW;
    int i;
    if((i=getaddrinfo(argv[1], NULL, &hints, &res))!=0){
        fprintf(stderr, "getaddrinfo failed, interpreting return status code: %s\n", gai_strerror(i));
        exit(EXIT_FAILURE);
    }
    for(struct addrinfo * i= res; i!=NULL; i=i->ai_next){
        char * IP=calloc(128, sizeof(char));
        void * ip=NULL;
        switch (i->ai_family) {
            case AF_INET:
                ip=&((struct sockaddr_in *) i->ai_addr)->sin_addr;
                break;
            case AF_INET6: 
                ip=&((struct sockaddr_in6 *) i->ai_addr)->sin6_addr;
                break;
        }
        inet_ntop(i->ai_family, ip, IP , 100);
        printf("%s\n",IP);
        free(IP);
    }
    freeaddrinfo(res);

}
