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

int manage_communication(int fd,fd_set *active_fds, fd_set *read_fds, int nfds, int socket_fd, sqlite3 *db)
{
    struct response resp;
    struct communication data;
    char responseToClient[100];
    char password[100];
    char username[100];
    int bytes;

    //Read a request from the client
    if ((bytes = read (fd, &data, sizeof(data))) < 0)
    {
        perror ("The data from client was not read\n");
        exit(EXIT_FAILURE);
    }

    if(data.communication_type == NOT_LOGGED) {
        //Split the buffer in username/password

        strcpy(username,strtok(data.message,"/"));
        strcpy(password,strtok(NULL,"/\n"));

        printf ("[SERVER]The username was received:%s\n\n", username);

        //Verify if the user exist in db
        int user_exist = verify_user_exist(db,username,password);
        int logged_status;

        //Exist-login
        if(user_exist > 0)
        {
            logged_status = get_logged_status(db,username);
            printf("Logged_status: %d", logged_status);
            if(logged_status == 0) {
                // The user is logged
                update_logged_status(db,username,password,"1");


                //Succesfull response 
                bzero(responseToClient,100);
                strcpy(responseToClient,"Welcome, ");
                strcat(responseToClient,username);
                strcat(responseToClient, "!");

                //Configure the response structure
                strcpy(resp.message,responseToClient);
                resp.status=1;

                if (bytes && (write(fd, &resp, sizeof(resp)) <0))
                {
                    perror ("[SERVER] The response was not send.\n");
                    exit(EXIT_FAILURE);
                }
            }
            else 
            {
                bzero(responseToClient,100);
                strcpy(responseToClient,"The user is already logged");
                strcpy(resp.message,responseToClient);
                resp.status=0;

                if(write(fd, &resp, sizeof(resp)) <0)
                {
                    perror ("[SERVER] The response was not send.\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
        else {
            bzero(responseToClient,100);
           
            //Doesn't exist
            strcpy(responseToClient,"The user doesn't exist");
            strcpy(resp.message,responseToClient);
            resp.status=0;

            if(write(fd, &resp, sizeof(resp)) <0)
            {
                perror ("[SERVER] The response was not send.\n");
                exit(EXIT_FAILURE);
            }
        }
        return bytes;
    } else if(data.communication_type == LOGGED)
    {
        // for (int file_desc = 5; file_desc <= nfds; file_desc++)
        // {
        //     if(file_desc != fd && file_desc != socket_fd)
        //     {
        //         if(bytes && (write(file_desc,&data,sizeof(data)) < 0))
        //         {
        //             perror("[SERVER] The message was not send.\n");
        //         }
        //     }
        // }

        // return bytes;
    }
    else if (data.communication_type == LOG_OUT)
    {
        update_logged_status(db,username,password,"0");
        printf ("[SERVER] S-a deconectat clientul cu descriptorul %d.\n",fd);
        fflush (stdout);
        close (fd);		/* inchidem conexiunea cu clientul */
        FD_CLR (fd, active_fds);/* scoatem si din multime */
    }
}

int main()
{
    sqlite3 *db; 
    char *errMsg = 0;

    int response = sqlite3_open("users.db",&db);

    if(response)
    {
        fprintf(stderr, "Error: %s\n", sqlite3_errmsg(db));
    }

    // create_table(db,"CREATE TABLE USERS("\
    //     "ID INT PRIMARY KEY NOT NULL," \
    //     "USERNAME VARCHAR(36) NOT NULL," \
    //     "PASSWORD VARCHAR(36) NOT NULL," \
    //     "STATUS INT NOT NULL," \
    //     "ID_GROUP INT NULL);" \
    // );

    // delete_account(db,"DROP TABLE USERS");

    // insert_user(db,"0","marius", "password1234", "0", "NULL");
    // insert_user(db,"1","pricope", "parola1234","0","NULL");
    // insert_user(db,"2","andrei", "password1234","0","NULL");

    select_table(db, "SELECT * FROM USERS;");


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
        //Copy the active descriptors to read descriptors
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
        //Verify if exist some socket ready
        for (int fd = 0; fd <= nfds; fd++)
        {
            //Verify if exist some socket ready to read
            if (fd != socket_fd && FD_ISSET (fd, &read_fds))
            {
                if (manage_communication(fd,&active_fds,&read_fds,nfds, socket_fd,db))
                {
                    
                }
            }
        }
    }
    printf("Ok!");
}