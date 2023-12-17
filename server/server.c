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

int client_fds[100];
int client_fds_lenght = 0;

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

struct request reaciving_request(int client_fd)
{
    struct request req;
    int bytes;
    if((bytes = read(client_fd,&req,sizeof(req)))<0)
    {
        perror("READING ERROR!");
        exit(EXIT_FAILURE);
    }

    return req;
}

void sending_response(int client_fd, char *response, int response_status)
{
    struct response res;
    strcpy(res.message,response);
    res.status=response_status;

    printf("Sending...\n");

    if (strlen(res.message) && write(client_fd, &res, sizeof(res)) < 0)
    {
        perror ("THE RESPONSE HAS NOT SEND!\n");
        exit(EXIT_FAILURE);
    }
}

void* communication_manager(void * client_socket)
{
    int client_socket_fd = *(int*)client_socket;
    struct request req;
    struct response res;
    //Open db
    sqlite3 *db = open_db("users.db"); 

    int bytes;
    while(1) {
        if(client_socket_fd > 0) {
            req = reaciving_request(client_socket_fd);

            printf("Request: %s\n", req.message);

            if(req.logging_status == NOT_LOGGED)
            {
                char username[1024];
                char password[1024];
                strcpy(username,strtok(req.message,"/"));
                strcpy(password,strtok(NULL,"/\n"));

                printf ("[SERVER]The username was received:%s\n\n", username);

                //Verify if the user exist in db
                int user_exist = verify_user_exist(db,username,password);
                int logged_status;
                char response_to_client[1024];

                //Exist-login
                if(user_exist > 0)
                {
                    logged_status = get_logged_status(db,username);
                    if(logged_status == 0) {
                        // The user is logged
                        printf("User %s doesn't connected!\n", username);
                        update_logged_status(db,username,password,"1");
                        printf("The user was not logged %s", username);
                        fflush(stdout);
                        //Succesfull response 
                        bzero(response_to_client,1024);
                        strcpy(response_to_client,username);

                        sending_response(client_socket_fd,response_to_client,SUCCESS);
                    }
                    else 
                    {
                        bzero(response_to_client,1024);
                        strcpy(response_to_client,"The user already logged!\n");
                        sending_response(client_socket_fd,response_to_client,FAILED);
                    }
                }
                else {
                
                    //Doesn't exist
                    bzero(response_to_client,1024);
                    strcpy(response_to_client,"The user doesn't exist\n");
                    sending_response(client_socket_fd,response_to_client,FAILED);
                }
            }
            else if(req.logging_status == LOGGED)
            {
                if(req.group_status == OUT_GROUP)
                {

                }
                else if (req.group_status == IN_GROUP)
                {
                    strcpy(res.message,req.message);
                    res.status=1;
                    for(int client_fd = 0 ; client_fd < client_fds_lenght; client_fd++)
                    {
                        if(client_fds[client_fd] != client_socket_fd) {
                        
                            write(client_fds[client_fd],&res,sizeof(res));
                        }
                    }
                }
            }

            
        }
    }

    return NULL;
}

void update_fd_clients_list(int client_fd)
{
    client_fds[client_fds_lenght] = client_fd;
    client_fds_lenght++;
}

int main()
{
    //Test
    sqlite3 *db = open_db("users.db"); 

    // create_table(db,"CREATE TABLE USERS("\
    //     "ID INT PRIMARY KEY NOT NULL," \
    //     "USERNAME VARCHAR(36) NOT NULL," \
    //     "PASSWORD VARCHAR(36) NOT NULL," \
    //     "STATUS INT NOT NULL," \
    //     "ID_GROUP INT NULL);" \
    // );

    // delete_account(db,"DROP TABLE USERS");

    update_logged_status(db,"pricope","parola1234", "0");
    update_logged_status(db,"marius","password1234", "0");
    update_logged_status(db,"andrei","password1234", "0");

    select_table(db, "SELECT * FROM USERS;");
    //Test


    //Setup the socket
    int server_socket_fd = setup_socket();

    printf("The server wait at port %d...\n", PORT);

    struct sockaddr_in client_address;
    int len = sizeof(struct sockaddr_in);

    pthread_t thread;
    while (1)
    {
        int client = accept(server_socket_fd,(struct sockaddr *)&client_address,&len);
        update_fd_clients_list(client);
        pthread_create(&thread,NULL,communication_manager, &client);
    }
    close(server_socket_fd);
    printf("Ok!");
}