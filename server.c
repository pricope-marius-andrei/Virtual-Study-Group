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
#include "utils/db.h"

#define PORT 1025
char email[100];

struct response
{
    int status;
    char message[100];
};

int setup_socket()
{
    //Create the socket file descriptor
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if(socket_fd == -1)
    {
        perror("Error at creating socket.");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;

    //Reuse the same address after close server
    int optval = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval,sizeof(optval));


    bzero (&server_addr, sizeof (server_addr));
    //Configure the socket
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT); //host to network byte
    server_addr.sin_addr.s_addr = INADDR_ANY; //allow to connect any type of interfaces

    if(bind(socket_fd,(struct sockaddr*) &server_addr, sizeof(server_addr)) == -1)
    {
        perror("Error at binding.");
        exit(EXIT_FAILURE);
    }

    if(listen(socket_fd,10) == -1)
    {
        perror("Error at listen connections.");
        exit(EXIT_FAILURE);
    }

    return socket_fd;
}

void update_active_list(int client, fd_set *active_fds, int *nfds)
{
    //Ajust the maximum number of descriptors
    if (*nfds < client) 
        *nfds = client;
            
    //Active list updated with new client
    FD_SET (client, active_fds);

    printf("[SERVER] The client with the descriptor %d was connected\n",client);
    fflush (stdout);
}

int logIn(int fd, char email[100])
{
    struct response resp;
    char responseToClient[100];
    char buffer[100];
    char password[100];
    int bytes;
    if ((bytes = read (fd, buffer, sizeof(buffer))) < 0)
    {
        perror ("The email was not read\n");
        exit(EXIT_FAILURE);
    }

    //Split the buffer in email/password

    strcpy(email,strtok(buffer,"/"));
    strcpy(password,strtok(NULL,"/\n"));

    printf ("[SERVER]The email was received:%s\n", email);
        
    /*Succesfull response */
    bzero(responseToClient,100);
    strcpy(responseToClient,"Log-in successfully with email: ");
    strcat(responseToClient,email);

    //Configure the response structure
    strcpy(resp.message,responseToClient);
    resp.status=1;
        
    if (bytes && write(fd, &resp, sizeof(resp)) <0)
    {
        perror ("[SERVER] The response was not send.\n");
        exit(EXIT_FAILURE);
    }
    
    return bytes;
}

int main()
{
    //Setup the socket
    int socket_fd = setup_socket();

    //The stucture for client adress
    struct sockaddr_in client_addr;

    //Configure descriptors pool
    fd_set active_fds;
    fd_set read_fds;

    FD_ZERO(&active_fds);
    FD_SET(socket_fd,&active_fds);

    int nfds = socket_fd;

    printf ("[SERVER] The server wait at %d port...\n", PORT);

    while (1)
    {
        //Copy the readable descriptors to active descriptors
        bcopy ((char *) &active_fds, (char *) &read_fds, sizeof (read_fds));

        if (select (nfds+1, &read_fds, NULL, NULL, NULL) < 0)
        {
            perror ("[SERVER] select() error.\n");
        }

        if (FD_ISSET (socket_fd, &read_fds))
        {
            int len = sizeof (client_addr);
            bzero (&client_addr, sizeof (client_addr));

            // Accept the connection with a client
            int client = accept (socket_fd, (struct sockaddr *) &client_addr, &len);

            if (client < 0)
            {
                perror ("[SERVER] accept() error\n");
                continue;
            }

            update_active_list(client,&active_fds,&nfds);
        }
        //Verify if exist some socket ready to send the credentials
        for (int fd = 0; fd <= nfds; fd++)
        {
            //Verify if exist some socket ready to read
            if (fd != socket_fd && FD_ISSET (fd, &read_fds))
            {	
                if (logIn(fd,email))
                {
                    printf ("[SERVER] S-a deconectat clientul cu descriptorul %d.\n",fd);
                    fflush (stdout);
                    close (fd);		/* inchidem conexiunea cu clientul */
                    FD_CLR (fd, &active_fds);/* scoatem si din multime */
                }
            }
        }
    }
    printf("Ok!");
}