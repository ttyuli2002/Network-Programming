/* hw2.c -- same as Operating System hw2 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/select.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define NAME_SIZE 1024
#define MAX_CLIENTS 5
struct PLAYER 
{
  int socket;
  char* username;
};

/* global var */
char* secret_w;
int max_fd;
struct PLAYER all_player[MAX_CLIENTS];
int player_count = 0;  // next free spot

// preprocess all input data
void preprocess(char* argv[], int* seed, int* port, char** dict_name, int* longest_word_length)
{
    *seed = atoi(argv[1]);
    
    *port = atoi(argv[2]);
    if (*port < 1024 || *port > 65535)
    {
      *port = 0;
    }

    *dict_name = argv[3];

    *longest_word_length = atoi(argv[4]);   
}

// read all Word lengths <= 1024 bytes, and <= [longest_word_length] into dictionary
char** read_dict(char* dict_name, int longest_word_length, int* dictionary_size)
{
    FILE* file = fopen(dict_name, "r");
    if(file == NULL)
    {
        fprintf(stderr, "ERROR: Can not open file.\n");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0L, SEEK_END);
    int txtbytes = ftell(file);
    fseek(file, 0L, SEEK_SET);  

    char** dict = calloc(txtbytes, sizeof(char*));
    if (dict == NULL)
    {
        fprintf(stderr, "ERROR: calloc failed.\n");
        exit(EXIT_FAILURE); 
    }    

    char ch;
    char* buffer;
    int max_word;
    int count = 0;
    *dictionary_size = 0;
    if (1024 < longest_word_length)
    {
        buffer = calloc(1024, sizeof(char));
        max_word = 1024;
    }
    else
    {
        buffer = calloc(longest_word_length, sizeof(char));
        max_word = longest_word_length;
    }

    while((ch = fgetc(file))!=EOF)
    {
        if (ch == '\n')
        {
            // if count <= max_word, store the word in dict
            if (count <= max_word)
            {
                buffer[count] = '\0';
                count += 1;
                dict[*dictionary_size] = calloc(count, sizeof(char));
                strcpy(dict[*dictionary_size], buffer);
                *dictionary_size += 1;
            }

            // reset buffer & count
            memset(buffer, 0, max_word);
            count = 0;
        }

        else
        {
            buffer[count] = ch;
            count += 1;
        }
    }

    fclose(file);
    free(buffer);

    return dict;
}

// find the secret word
char* find_Secret(char** dict, int dictionary_size)
{
    int index = rand() % dictionary_size;

    return dict[index];
}

/*
 *  check if the user input name exists in username list
 *  username: a list of string that stored existing user names
 *  player_count: number of users currently playing the game
 *  buffer: a string that stores user input word
 */
bool name_exist(char* buffer)
{
    if (player_count == 0)
    {
        return false;
    }

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (all_player[i].username != NULL && strcasecmp(all_player[i].username, buffer) == 0)
        {
            return true;
        }
    }

    return false;
}

/*
 *  check # of different char in word & # occurence of each char (act as set)
 *  word: user input word
 *  dict[]: a list stored different char appeared in word (work as set)
 *  count[]: a list stored # occurence of each char appeared in word
*/ 
int duplicate_char(char* word, char dict[], int count[])
{
    int i, j;
    int dict_count = 0;
    memset(count, 0, strlen(word) * sizeof(int));
    for (i = 0; i < strlen(word); i++)
    {
        bool find = false;
        char temp = tolower(word[i]);
        for (j = 0; j < dict_count; j++)
        {
            if (temp == dict[j])
            {
                find = true;
                count[j] += 1;
                break;
            }
        }

        if (!find)
        {
            dict[dict_count] = temp;
            count[dict_count] = 1;
            dict_count += 1;            
        }
    }

    return dict_count;
}

/* 
 *  compare # correct position & # correct char betweensecret word & user input word
 *  output would be stored in correct_char & correct_position
*/ 
void compare(char* secret_w, char secrect_dict[], int secrect_count[], int secrect_num, char* buffer, char temp_dict[], int temp_count[], 
        int temp_num, int* correct_char, int* correct_position)
{
    int i, j;
    for (i = 0; i < secrect_num; i++)
    {
        for (j = 0; j < temp_num; j++)
        {
            if (secrect_dict[i] == temp_dict[j])
            {
                if (secrect_count[i] <= temp_count[j])
                {
                    *correct_char += secrect_count[i];
                    break;
                }
                else
                {
                    *correct_char += temp_count[j];
                    break;
                }
            }
        }
    }

    for (i = 0; i < strlen(secret_w); i++)
    {
        if (secret_w[i] == buffer[i])
        {
            *correct_position += 1;
        }
    }
}

/*
 * set up server
 * port_num: # port that server suppose to bind to
 * return: socket descripter
*/
int setup_server(int port_num)
{
    if (port_num > 65535 || port_num <= 0) 
    {
        // ensure that the port number is valid
        perror("INVALID PORT NUMBER");
        exit(EXIT_FAILURE);
        // errors if it isnt
    }
    struct sockaddr_in servaddr;
    // defines server struct
    int server_socket;
    // defines integer for the server socket descriptor
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    // request a socket from the OS for TCP/IP use
    if (server_socket == -1) 
    {
        // if it fails
        perror("SOCKET CREATION FAILED");
        exit(EXIT_FAILURE);
    }
    servaddr.sin_family = AF_INET;
    // setting the adress family
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // setting the IP adress
    servaddr.sin_port = htons(port_num);
    // setting the port
    if ((bind(server_socket, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) 
    {
        // attempting to bind the server to a given port an IP address
        perror("BIND FAILED");
        exit(EXIT_FAILURE);
        // binding failed
    }

    if ((listen(server_socket, 5)) != 0) 
    {
        // attempting to have the socket listen on the port and allow a maximum of 5
        // people to wait
        perror("LISTEN FAILED");
        exit(EXIT_FAILURE);
        // If it fails
    }
    
    return server_socket;
    // returns the server socket
}

/*
 * accept single client connection
*/
void accept_connection(int sock) {
    struct sockaddr_in player;
    socklen_t player_len = sizeof( player );
    int new_player = accept( sock, (struct sockaddr *)&player, (socklen_t *)&player_len );
    printf( "Successfully Accepted Player Connection\n" );

    // include user socket into the array
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (all_player[i].socket == 0) 
        {
            all_player[i].socket = new_player;
            if (new_player > max_fd)
            {
                max_fd = new_player;
            }
            all_player[i].username = NULL;
            break;
        }
    }

    // requestion user name
    char* greeting = "Welcome to Guess the Word, please enter your username.\n";
    int n = send( all_player[player_count].socket, greeting, strlen(greeting), 0 );
    if ( n < 0 )
    {
        perror( "ERROR: send() failed\n" );
    } 
}

/*
 * disconnect client socket based on the giver array index
*/
void disconnect_sock(int i)
{
    printf("player %d disconnect\n", all_player[i].socket);
    close(all_player[i].socket);

    all_player[i].socket = 0;
    if (all_player[i].username != NULL) 
    {
        free(all_player[i].username);
    }
    all_player[i].username = NULL;
    player_count--;
}

/*
 *  run the server
 *  port_number: server port number, used to bind the socket
 *  longest_word_length: max possible length of secret word selected 
 *  secret_w: selected secret word
 *  secrect_dict: a list stored different char appeared in secret_w (work as set)
 *  secrect_count: a list stored # occurence of each char appeared in secret_w
 *  dict: a list of string stores all the words available for secret_w
 *  dictionary_size: length of dic
*/
void socket_server(int sock, int longest_word_length, char* secret_w, char secrect_dict[], int secrect_count[], int secrect_num, char** dict, int dictionary_size)
{
    fd_set SOCKETS;
    max_fd = sock;
    int i, n;
    while (1)
    {
        FD_ZERO( &SOCKETS );
        FD_SET( sock, &SOCKETS );

        // if detected new player connection, add to player_socks
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (all_player[i].socket != 0) 
            {
                FD_SET(all_player[i].socket, &SOCKETS);
            }
        }

        // BLOCK until activties
        int ready = select(max_fd + 1 , &SOCKETS, NULL, NULL, NULL );
        if (ready < 0 )
        {
            perror("ERROR: selct() failed\n");
        }
        if ( FD_ISSET( sock, &SOCKETS ) ) 
        {
            accept_connection(sock);
        }

        for (i = 0; i < MAX_CLIENTS; i++)
        {
            if (all_player[i].socket != 0 && FD_ISSET(all_player[i].socket, &SOCKETS))
            {
                char* buffer = calloc(NAME_SIZE, sizeof(char));
                n = recv( all_player[i].socket, buffer, NAME_SIZE - 1, 0 );
                if ( n < 0 )  
                {
                    free(buffer);
                    perror("recv() FAILED");
                    exit(EXIT_FAILURE);
                }   
                else if ( n == 0 )
                {
                    disconnect_sock(i);
                }
                else
                {
                    buffer[n - 1] = '\0';
                    // get username
                    if (all_player[i].username == NULL)
                    {
                        if (name_exist(buffer))
                        {
                            char renaming[1024] = "Username ";
                            strcat(renaming, buffer);
                            strcat(renaming, " is already taken, please enter a different username\n");

                            send( all_player[i].socket, renaming, strlen(renaming), 0 );
                            free(buffer);

                            continue;
                        }

                        all_player[i].username = buffer;
                        printf("added username: %s\n", all_player[i].username);

                        char welcome[1024] = "Let's start playing, ";
                        strcat(welcome, all_player[i].username);
                        strcat(welcome, "\n");
                        send( all_player[i].socket, welcome, strlen(welcome), 0 );

                        // notify every player
                        char notice[1024] = "There are ";
                        char temp[2];
                        sprintf(temp, "%d", (player_count + 1));
                        strcat(notice, temp);
                        strcat(notice, " player(s) playing. The secret word is ");
                        char temp2[2];
                        int secret_len = strlen(secret_w);
                        sprintf(temp2, "%d", secret_len);
                        strcat(notice, temp2);
                        strcat(notice, " letter(s).\n");
                        send( all_player[i].socket, notice, strlen(notice), 0 );
                        
                        player_count += 1;
                    }
                    // guess word
                    else
                    {
                        printf("client: %s, guess: %s, secret is %s, is it same? %d\n", all_player[i].username, buffer, secret_w, strcasecmp(secret_w, buffer));

                        // if not same len, send error message
                        if (strlen(buffer) != strlen(secret_w))
                        {
                            char error_msg[1024] = "Invalid guess length. The secret word is ";
                            char temp[5];
                            sprintf(temp, "%ld", strlen(secret_w));
                            strcat(error_msg, temp);
                            strcat(error_msg, " letter(s).\n");
                            send( all_player[i].socket, error_msg, strlen(error_msg), 0 );
                            continue;
                        }

                        // if same word, send notice & disconnect all user
                        if (strcasecmp(secret_w, buffer) == 0)
                        {
                            printf("same word!\n");
                            char notice[1024];
                            strcpy(notice, all_player[i].username);
                            strcat(notice, " has correctly guessed the word ");
                            strcat(notice, secret_w);
                            strcat(notice, "\n");
                            int j;
                            for (j = 0; j < MAX_CLIENTS; j++)
                            {
                                if (all_player[j].socket != 0)
                                {
                                    send( all_player[j].socket, notice, strlen(notice), 0 );
                                    disconnect_sock(j);                                    
                                }
                            }
                            player_count = 0;

                            // select a new word
                            secret_w = find_Secret(dict, dictionary_size);
                            memset(secrect_dict, 0, strlen(secret_w) * sizeof(char));
                            memset(secrect_count, 0, strlen(secret_w) * sizeof(int));
                            secrect_num = duplicate_char(secret_w, secrect_dict, secrect_count);

                            printf("new word is %s\n", secret_w);
                            for (int i = 0; i < secrect_num; i++)
                            {
                                printf("char is %c, num is %d\n", secrect_dict[i], secrect_count[i]);
                            }

                            continue;
                        }

                        // if anyuser guessed an incorrect word
                        int len = strlen(buffer);
                        char temp_dict[len];
                        int temp_count[len];
                        memset(temp_dict, 0, len);
                        memset(temp_count, 0, len);
                        printf("before:\n");
                        for (int un = 0; un < strlen(buffer); un++)
                        {
                            printf("char is %c, count is %d\n", temp_dict[un], temp_count[un]);
                        }
                        int temp_num = duplicate_char(buffer, temp_dict, temp_count);
                        printf("buffer is %s\n", buffer);
                        for (int un = 0; un < temp_num; un++)
                        {
                            printf("char is %c, count is %d\n", temp_dict[un], temp_count[un]);
                        }


                        int correct_char = 0;
                        int correct_position = 0;
                        compare(secret_w, secrect_dict, secrect_count, secrect_num, buffer, temp_dict, temp_count, temp_num, &correct_char, &correct_position);

                        char temp_guess[1024];
                        strcpy(temp_guess, all_player[i].username);
                        strcat(temp_guess, " guessed ");
                        strcat(temp_guess, buffer);
                        strcat(temp_guess, ": ");
                        char temp[5];
                        sprintf(temp, "%d", correct_char);
                        strcat(temp_guess, temp);
                        strcat(temp_guess, " letter(s) were correct and ");
                        memset(temp, 0, 5);
                        sprintf(temp, "%d", correct_position);
                        strcat(temp_guess, temp);
                        strcat(temp_guess, " letter(s) were correctly placed.\n");

                        int j;
                        for (j = 0; j < MAX_CLIENTS; j++)
                        {
                            if (all_player[j].socket != 0)
                            {
                                send( all_player[j].socket, temp_guess, strlen(temp_guess), 0 );
                            }
                        }
                    }
                }
            }
        }

    }
}

int main(int argc, char* argv[])
{
    setvbuf( stdout, NULL, _IONBF, 0 );

    // check input argument
    if (argc != 5)
    {
        perror( "not enough argumen: ./a.out [seed] [port] [dictionary_file] [longest_word_length]\n" );
        return EXIT_FAILURE;
    }

    int seed = 0;;
    int port_num = 0;;
    char* dict_name = calloc(128, sizeof(char));
    int longest_word_length = 0;
    preprocess(argv, &seed, &port_num, &dict_name, &longest_word_length);

    int dictionary_size = 0;
    char** dict = read_dict(dict_name, longest_word_length, &dictionary_size);
    srand((unsigned int) seed);

    char* secret_w = find_Secret(dict, dictionary_size);
    char secrect_dict[strlen(secret_w)];
    int secrect_count[strlen(secret_w)];
    int secrect_num = duplicate_char(secret_w, secrect_dict, secrect_count);
    for (int i = 0; i < secrect_num; i++)
    {
        printf("char is %c, num is %d\n", secrect_dict[i], secrect_count[i]);
    }
    int server_socket = setup_server(port_num);
    socket_server(server_socket, longest_word_length, secret_w, secrect_dict, secrect_count, secrect_num, dict, dictionary_size);

    // free all variables
    for (int i = 0; i < dictionary_size; i++)
    {
        free(dict[i]);
    }
    free(dict);

    return EXIT_SUCCESS;
}