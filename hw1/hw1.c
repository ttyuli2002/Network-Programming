/* 
    HW1:
    Implementation of TFTP Server in "Octet" Mode, allow receiving file for <= 32 MB.
*/

#include "unp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUF_LEN 516
#define DATA_LEN 512
#define ACK_LEN 4

#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERROR 5

FILE* fp;   // file descriptor for reading/ writing to buffer
int allcli = 0; // count to see if all chile process are finished

/// @brief handling finished children process
/// @param signal The signal that we are handling 
void signalHandler(int signal)                                                                                                                                                                                        
{
    if (signal == SIGCHLD)
    {
        pid_t pid;
        int stat;
        // wait for all child to finish
        while ((pid=waitpid(-1, &stat, WNOHANG))>0) {
            printf("Parent sees child PID %d has terminated\n\n", pid);
            allcli--;
        }
    }
}

/// @brief getting the OpCode from client message
/// @param buffer string that stored client's message
/// @return the Opcode of client's message
unsigned short int get_Opcode (char* buffer)
{
    unsigned short int opcode;
    unsigned short int * opcode_ptr;
    opcode_ptr = (unsigned short int *)buffer;
    opcode = ntohs(*opcode_ptr);

    return opcode;
}

/// @brief getting the Blocknumber from client message
/// @param buffer string that stored client's message
/// @return the block number of client's message
unsigned short int get_Blocknum(char* buffer)
{
    unsigned short int * opcode_ptr;
    opcode_ptr = (unsigned short int *)buffer;
    return ntohs(*(opcode_ptr + 1));   
}

/// @brief getting the file name from client's RRQ/WRQ
/// @param buffer string that stored client's message
/// @param Filename file buffer that needs to store information from buffer
/// @param Mode mode buffer that needs to store information from buffer
/// @param n total byte of the buffer
void get_FileName(char* buffer, char** Filename, char** Mode, int n)
{
    memset(*Filename, 0, BUF_LEN);
    memset(*Mode, 0, BUF_LEN);

    int count = 0;
    int zero_time = 0;
    char* temp = calloc(BUF_LEN, sizeof(char));
    memset(temp, 0, BUF_LEN);

    // first 2 bytes are OpCode
    for (int i = 2; i < n + 1; i++)
    {
        // filename finishes
        if ((zero_time == 0) && (buffer[i] == '\0'))
        {
            temp[count] = '\0';
            strncpy(*Filename, temp, BUF_LEN);
            memset(temp, 0, BUF_LEN);
            zero_time ++;
            count = 0;
            continue;
        }
        // Mode finishes
        else if (buffer[i] == '\0')
        {
            temp[count] = '\0';
            strncpy(*Mode, temp, count);
            return;
        }

        temp[count] = buffer[i];
        count ++;
    }   
}

/// @brief getting the data from client's DATA packet
/// @param buffer string that stored client's message
/// @param Data_buf Data buffer that needs to store data from buffer
/// @param n total byte of the buffer
void get_data(char* buffer, char* Data_buf, int n)
{
    int count = 0;
    // first 4 bytes are OpCode and block number
    for (int i = 4; i < n; i++)
    {
        *(Data_buf + count) = buffer[i];
        count ++;
    }
    *(Data_buf + count) = '\0';
}

/// @brief getting the block number we should reply to the client based on their Opcode and block number
/// @param Opcode Client's last socket's Opcode
/// @param block_num Client's last socket's Block number
/// @return correct blocknumber we should sent to the client
int reply_blocknum(int Opcode, unsigned short int block_num)
{
    switch (Opcode)
    {
    case RRQ:
        return 1;
    case WRQ:
        return 0;
    case DATA:
        return block_num;
    case ACK: 
        return block_num + 1;

    default:
        return (unsigned short int) -1;
    }
}

/// @brief compose and send the ACK socket to the client
/// @param sock_fd file descriptor for the udp socket that we will use to send the acknowlegment
/// @param Opcode Client's last socket's Opcode
/// @param ori_blocknum Client's last socket's Block number
/// @param cliaddr client address for socket
/// @param len length of client address for socket
void send_ACK(int sock_fd, int Opcode, int ori_blocknum, struct sockaddr_in cliaddr, socklen_t len)
{
    char* buffer = calloc(ACK_LEN, sizeof(char));

    int blocknum = reply_blocknum(Opcode, ori_blocknum);
    if (blocknum < 0)
    {
        perror( "ERROR: get_blocknum() failed\n");
    }
    memset(buffer, 0, ACK_LEN);

    // ACK OpCode is 4
    uint16_t op = htons(ACK);
    memcpy(buffer, &op, 2);

    uint16_t bn = htons(blocknum);
    memcpy(buffer + 2, & bn, 2);

    // send ACK back to server
    int n;
    n = sendto(sock_fd, (char *)buffer, ACK_LEN, MSG_CONFIRM, ( struct sockaddr *) &cliaddr, len);
    if ( n < 0 )
    {
        perror( "ERROR: sendto() failed\n" );
    }
}

/// @brief compose and send the DATA socket to the client
/// @param sock_fd file descriptor for the udp socket that we will use to send the acknowlegment
/// @param Opcode Client's last socket's Opcode
/// @param blocknum Client's last socket's Block number
/// @param cliaddr client address for socket
/// @param len length of client address for socket
/// @param conti flag for whether to resend DATA socket (conti = 0 or 1)
/// @return number of byte read in to the buffer
int send_DATA(int sock_fd, int Opcode, int blocknum, struct sockaddr_in cliaddr, socklen_t len, int conti)
{
    char* buffer = calloc(BUF_LEN, sizeof(char));
    memset(buffer, 0, ACK_LEN);

    // DATA OpCode is 3
    uint16_t op = htons(DATA);
    memcpy(buffer, &op, 2);

    uint16_t bn = htons(blocknum);
    memcpy(buffer + 2, & bn, 2);

    char* data_buf = calloc(DATA_LEN, sizeof(char));

    // if not recv ACK from client, move back where it lastly started
    if (conti == 1)
    {
        fseek(fp, -512L, SEEK_CUR);
    }
    int bytesRead = fread(data_buf, 1, DATA_LEN, fp);
    memcpy(buffer + 4, data_buf, bytesRead);

    printf("data sent: %d bytes\n %s\n", bytesRead, data_buf);
    
    // send DATA back to server
    int n;
    n = sendto(sock_fd, (char *)buffer, 4 + bytesRead, MSG_CONFIRM, ( struct sockaddr *) &cliaddr, len);
    if ( n < 0 )
    {
        perror( "ERROR: sendto() failed\n" );
    }

    return bytesRead;
}

/// @brief send error to the client
/// @param sock_fd file descriptor for the udp socket that we will use to send the acknowlegment
/// @param Error_code error code for sending the error socket
/// @param cliaddr client address for socket
/// @param len length of client address for socket
void send_error(int sock_fd, int Error_code, struct sockaddr_in cliaddr, socklen_t len)
{
    char* buffer = calloc(5, sizeof(char));
    memset(buffer, 0, 5);
    // create OpCode
    uint16_t op = htons(ERROR);
    memcpy(buffer, &op, 2);
    // Error Code
    uint16_t bn = htons(Error_code);
    memcpy(buffer + 2, & bn, 2);
    *(buffer + 4) = '\0';

    // send Error back to server
    int n;
    n = sendto(sock_fd, (char *)buffer, ACK_LEN + 1 , MSG_CONFIRM, ( struct sockaddr *) &cliaddr, len);
    if ( n < 0 )
    {
        perror( "ERROR: sendto() failed\n" );
    }
}

/// @brief preprocessing user input
/// @param argv user input arguments
/// @param start_port starting port that needs to store information from buffer
/// @param end_port ending port that that to store information from buffer
/// @param main_port main listenting port that needs to store information from buffer
void preprocess(char* argv[], int* start_port, int* end_port, int* main_port)
{
    *start_port = atoi(argv[1]);
    *end_port = atoi(argv[2]);
    *main_port = *start_port;
}

/// @brief bind the socket to the specific port
/// @param curr_port port that needs to bind to the socket
/// @return return the socket file descriptor
int bind_Sock(int curr_port)
{
    int sockfd;
    struct sockaddr_in saddr;

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("ERROR: socket() failed");
		exit(EXIT_FAILURE);
	}

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET; // IPv4
	saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(curr_port);

    if ( bind(sockfd, (const struct sockaddr *)&saddr, sizeof(saddr)) < 0 )
	{
		perror("ERROR: bind() failed");
		exit(EXIT_FAILURE);
	}
    
    printf("TFTP Server started.\n" );

    printf( "Server IP: %s\n", inet_ntoa(saddr.sin_addr) );
    printf( "Server port#: %d\n", ntohs( saddr.sin_port ) );

    return sockfd;
}

/// @brief reply to WRQ request and send ACK for incoming DATA sockets
/// @param buffer buffer that stored WRQ socket
/// @param Opcode OpCode for the WRQ socket
/// @param cliport client port #
/// @param serport server port #
/// @param buffer_len length of buffer
/// @param cliaddr client address for socket
/// @param len length of client address for socket
void Reply_WRQ(char* buffer, int Opcode, int cliport, int serport, int buffer_len, struct sockaddr_in cliaddr, socklen_t len)
{
    pid_t p = fork();
    if ( p < 0 )
    {
        perror("ERROR: fork() failed");
        exit(EXIT_FAILURE);
    }
    if (p > 0) // parent process
    {
        return;
    }
    
    printf("WRQ child pid: %d\n", getpid());
    char* Filename = calloc(BUF_LEN, sizeof(char));
    char* Mode = calloc(BUF_LEN, sizeof(char));
    get_FileName(buffer, &Filename, &Mode, buffer_len);

    unsigned short int bn = get_Blocknum(buffer);
    int blocknum = reply_blocknum(Opcode, bn);
    if (blocknum < 0)
    {
        perror( "ERROR: get_blocknum() failed\n");
    }

    printf("FileName: %s\n", Filename);
    fp = fopen(Filename, "w");
    if (fp == NULL)
    {
        perror( "ERROR: fopen() failed" );
        exit(EXIT_FAILURE);
    }

    int n;
    char* buffer2 = calloc(BUF_LEN, sizeof(char));
    char* Data_buf = calloc(DATA_LEN, sizeof(char));
    memset(buffer2, 0, BUF_LEN);
    memset(Data_buf, 0, DATA_LEN);
    int newsockfd = bind_Sock(serport);
    n = DATA_LEN;
    setvbuf(fp, NULL, _IONBF, 0);

    // blocked out timeout portion
    //struct timeval timeout={10 ,0}; //set timeout for 10 seconds
    /* set receive UDP message timeout */
    //setsockopt(newsockfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));

    while (1)
    {
        // send ACK (put on first to reply to WRQ as well)
        send_ACK(newsockfd, Opcode, blocknum, cliaddr, len);
        if (n < DATA_LEN)
        {
            fclose(fp);
            exit(EXIT_SUCCESS);
        }

        n = recvfrom(newsockfd, buffer2, BUF_LEN, 0, ( struct sockaddr *) &cliaddr, &len);
        /*
        if (n < 0)
        {
            //if  not  heard  from  the  other  party  for  10  seconds
            // abort  the  connection. 
            fclose(fp);
            close(newsockfd);
            exit(EXIT_FAILURE);
        }
        */

       // get data from buffer2 to Data_bufer for length n of received message
        get_data(buffer2, Data_buf, n);
        Opcode = get_Opcode(buffer2);
        blocknum = reply_blocknum(Opcode, get_Blocknum(buffer2));
        fprintf( fp, "%s", Data_buf );  // put the data into file descriptor
    }
}

/// @brief reply to RRQ request and send DATA for incoming ACK sockets
/// @param buffer buffer that stored WRQ socket
/// @param Opcode OpCode for the WRQ socket
/// @param cliport client port #
/// @param serport server port #
/// @param buffer_len length of buffer
/// @param cliaddr client address for socket
/// @param len length of client address for socket
void Reply_RRQ(char* buffer, int Opcode, int cliport, int serport, int buffer_len, struct sockaddr_in cliaddr, socklen_t len)
{
    pid_t p = fork();
    if ( p < 0 )
    {
        perror("ERROR: fork() failed");
        exit(EXIT_FAILURE);
    }
    if (p > 0) // parent process
    {
        return;
    }

    printf("RRQ child pid: %d\n", getpid());

    // put this RRQ into a new port for child process
    int newsockfd = bind_Sock(serport);
    
    char* Filename = calloc(BUF_LEN, sizeof(char));
    char* Mode = calloc(BUF_LEN, sizeof(char));
    get_FileName(buffer, &Filename, &Mode, buffer_len);

    unsigned short int bn = get_Blocknum(buffer);

    int blocknum = reply_blocknum(Opcode, bn);
    if (blocknum < 0)
    {
        perror( "ERROR: get_blocknum() failed\n");
    }

    printf("FileName: %s\n", Filename);
    fp = fopen(Filename, "r");
    if(fp == NULL)
    { 
        //  code 1: File not found.
        send_error(newsockfd, 1, cliaddr, len);
        exit(EXIT_FAILURE);
    }
    setvbuf(fp, NULL, _IONBF, 0);
    fseek(fp, 0, SEEK_SET);

    int n;
    //int n2;
    int cont = 0;
    char* buffer2 = calloc(BUF_LEN, sizeof(char));
    memset(buffer2, 0, BUF_LEN);

    // blocked out timeout portion
    //struct timeval timeout={1 ,0}; //set timeout for 1 seconds
    /* set receive UDP message timeout */
    //setsockopt(newsockfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));

    while (1)
    {
        // send data (put frist to reply for RRQ request) 
        n = send_DATA(newsockfd, Opcode, blocknum, cliaddr, len, cont);

        recvfrom(newsockfd, buffer2, BUF_LEN, 0, ( struct sockaddr *) &cliaddr, &len);
        //n2 = recvfrom(newsockfd, buffer2, BUF_LEN, 0, ( struct sockaddr *) &cliaddr, &len);
        /*
        if (n2 < 0)
        {
            // Upon  not  receiving  data  for  1  second,  
            // your  sender  should  retransmit  its  last  packet.
            cont = 1;
            continue;
        }*/

        // close condition: if data sent < 512 and received ACK from client
        if (n < 512)
        {
            fclose(fp);
            close(newsockfd);
            exit(EXIT_SUCCESS);
        }
        
        Opcode = get_Opcode(buffer2);
        blocknum = reply_blocknum(Opcode, get_Blocknum(buffer2));
    }
}

/// @brief listening to the main port for WRQ/RRQ request
/// @param sockfd file descriptor for the udp socket that we will use to send the acknowlegment
/// @param start_port starting port #
/// @param end_port ending port #
void listen_cli(int sockfd, int start_port, int end_port)
{
    int n;
    int cli_count = 0;  // # of CTID currently have
    socklen_t len;
    char* buffer = calloc(BUF_LEN, sizeof(char));
    struct sockaddr_in cliaddr;
    memset(&cliaddr, 0, sizeof(cliaddr));
	len = sizeof(cliaddr);

    while ( 1 )
    {
        // listening for any incoming data
        n = recvfrom(sockfd, buffer, BUF_LEN, 0, ( struct sockaddr *) &cliaddr, &len);
        if ( n < 0 )
        {
            printf( "ERROR: recvfrom() failed\n" );
            continue;
        }   
        buffer[n] = '\0';

        unsigned short int Opcode = get_Opcode(buffer);
        int temp_cliport = (int) ntohs(cliaddr.sin_port);
        int temp_serport = start_port + cli_count;
        if (temp_serport > end_port)
        {
            return;
        }
        cli_count++;
        allcli++;
        printf("curr cliport: %d, curr serport: %d, n cli: %d\n", temp_cliport, temp_serport, cli_count);

        if (Opcode == RRQ)
        {
            Reply_RRQ(buffer, Opcode, temp_cliport, temp_serport, n, cliaddr, len);
        }
        else if (Opcode == WRQ)
        {
            Reply_WRQ(buffer, Opcode, temp_cliport, temp_serport, n, cliaddr, len);
        }

    }

}

int main(int argc, char *argv[]) 
{
    // error checking
    if (argc != 3) {
        printf("wrong input arguments: ./tftp.out [start_port] [end_port]\n");
        return EXIT_FAILURE;
    }

    signal(SIGCHLD, signalHandler);
    setvbuf (stdout, NULL, _IONBF, BUFSIZ); // _IONBF for  no  buffering.

    // process input data
    int start_port = 0;
    int end_port = 0;
    int main_port = 0;
    preprocess(argv, &start_port, &end_port, &main_port);

    int sockfd = bind_Sock(main_port);
    listen_cli(sockfd, start_port + 1, end_port);

    while ( allcli != 0);

    return EXIT_SUCCESS;
}
