/*
    Steps:
    1. create socket
    2. configure the socket
    3. do binding
    4. listen the connections
    5. accept the new client
    6. get the input from the client and sent a output to it
*/
#include <error.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include "../utils/db.h"
#include "../utils/constants.h"
#include <pthread.h>

// struct handle_data 
// {
//   int fd;
//   fd_set *active_fds;
//   fd_set *read_fds;
//   int nfds;
//   int socket_fd;
//   sqlite3 *db;
// };

sqlite3* open_db(const char *file_path)
{
    sqlite3* db;
    char *errMsg = 0;

    int response = sqlite3_open("users.db",&db);

    if(response)
    {
        fprintf(stderr, "Error: %s\n", sqlite3_errmsg(db));
    }

    return db;
}

int setup_socket()
{
    //Create the socket file descriptor
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if(socket_fd == -1)
    {
        perror("CREATING SOCKET ERROR!");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_adress;

    //Reuse the same address after close server
    int optval = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval,sizeof(optval));


    bzero (&server_adress, sizeof (server_adress));
    //Configure the socket
    server_adress.sin_family = AF_INET;
    server_adress.sin_port = htons(PORT); //host to network byte
    server_adress.sin_addr.s_addr = INADDR_ANY; //allow to connect any type of interfaces

    if(bind(socket_fd,(struct sockaddr*) &server_adress, sizeof(struct sockaddr_in)) == -1)
    {
        perror("BINDING ERROR!");
        exit(EXIT_FAILURE);
    }

    if(listen(socket_fd, CLIENT_LISTENS) == -1)
    {
        perror("LISTENING ERROR!");
        exit(EXIT_FAILURE);
    }

    return socket_fd;
}

void *accepting_client(void *client_structure)
{
    struct client_connected* client_conn = (struct client_connected*)client_structure;
    int len = sizeof(struct sockaddr_in);
    while (1)
    {
        printf("smth");
        int client = accept (client_conn->socket, (struct sockaddr *) &client_conn->client_adress, &len);
        if(client == -1)
        {
            perror("THE CLIENT DOESN'T ACCEPTED!");
            exit(EXIT_FAILURE);
        }
        else 
        {
            printf("The client was accepted");
            exit(EXIT_SUCCESS);
        }
    }
}

int main()
{
    //Open db
    sqlite3 *db = open_db("users.db"); 

    //Setup the socket
    int server_socket_fd = setup_socket();

    //The stucture for client address
    struct sockaddr_in client_addr;


    printf("The server wait at port %d...\n", PORT);
    

    while (1)
    {

        int len = sizeof(struct sockaddr_in);
        int client = accept (server_socket_fd, (struct sockaddr *) &client_addr, &len);
        if(client == -1)
        {
            perror("THE CLIENT DOESN'T ACCEPTED!");
            exit(EXIT_FAILURE);
        }

        char buffer[1024];
        if(read(client,buffer,sizeof(buffer))<0)
        {
            perror("READING ERROR!");
            exit(EXIT_FAILURE);
        }

        printf("Message from client: %s", buffer);
        fflush(stdout);
    }
        
    
    printf("Ok!");
}