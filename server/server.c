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
#include <fcntl.h>

struct client client_fds[100];
int client_fds_lenght = 0;

void remove_client_fd(int client_fd)
{
    int remove_index = client_fds_lenght;
    for (int i = 0; i < client_fds_lenght; i++)
    {
        if(client_fds[i].fd == client_fd)
        {
            remove_index = i;
        }
    }

    printf("Remove index:%d\n",remove_index);

    for (int i = remove_index; i < client_fds_lenght - 1; i++)
    {
        client_fds[i] = client_fds[i+1];
    }

    client_fds_lenght--;
    close(client_fd);
    // for (int i = 0; i < client_fds_lenght; i++)
    // {
    //     printf("Client: %d\n", client_fds[i].fd);
    // }
}

int fd_exist(int client_fd)
{
    for (int i = 0; i < client_fds_lenght; i++)
    {
        if(client_fds[i].fd == client_fd)
        {
            return 1;
        }
    }

    return 0;
}

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
    if((bytes = read(client_fd,&req,sizeof(req))) <=0)
    {
        //remove the client fd from the list
        // remove_client_fd(client_fd);
        strcpy(req.message,"false");
    }

    return req;
}

void sending_response(int client_fd, int user_id, int group_id, char *response, char *file_content, int response_status)
{
    struct response res;
    strcpy(res.message,response);
    res.status=response_status;
    res.user_id = user_id;
    res.group_id = group_id;
    strcpy(res.file_content,file_content);

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
    int user_id = -1;
    char username[100];
    int group_id = -1;
    struct request req;
    struct response res;
    //Open db
    sqlite3 *db = open_db("users.db"); 

    char response_to_client[1024];
    int bytes;


    while(1) {
        if(client_socket_fd > 0) {
            req = reaciving_request(client_socket_fd);

            // printf("Request: %s\n", req.message);
            if(req.logging_status == NOT_LOGGED)
            {
                char password[100];
                strcpy(username,strtok(req.message,"/"));
                strcpy(password,strtok(NULL,"/\n"));

                printf ("[SERVER]The username was received:%s\n\n", username);

                //Verify if the user exist in db
                int user_exist = verify_user_exist(db,username,password);
                int logged_status;

                //Exist-login
                if(user_exist > 0)
                {
                    char condition[1024];
                    sprintf(condition, "WHERE USERNAME='%s'", username);
                    logged_status = get_field_value(db,condition,"STATUS","USERS");
                    if(logged_status == 0) {
                        user_id = get_field_value(db,condition, "ID", "USERS");
                        update_users_field(db,"STATUS",user_id,"1");
                        client_fds[client_fds_lenght++].fd = client_socket_fd;
                        printf("User with id: %d was connected!\n",user_id);
                        
                        
                        fflush(stdout);
                        //Succesfull response 
                        bzero(response_to_client,1024);
                        strcpy(response_to_client,username);
                        sending_response(client_socket_fd,user_id,-1,response_to_client, "",SUCCESS);
                    }
                    else 
                    {
                        bzero(response_to_client,1024);
                        strcpy(response_to_client,"The user already logged!\n");
                        sending_response(client_socket_fd,user_id,-1,response_to_client,"",FAILED);
                    }
                }
                else {
                
                    //Doesn't exist
                    bzero(response_to_client,1024);
                    strcpy(response_to_client,"The user doesn't exist\n");
                    sending_response(client_socket_fd,user_id,-1,response_to_client, "",FAILED);
                }
            }
            else if(req.logging_status == LOGGED)
            {
                if(req.gr_info.group_status == OUT_GROUP)
                {
                    if(req.gr_info.group_connection == CREATE_GROUP)
                    {
                        char name[1024];
                        char password[1024];
                        strcpy(name,strtok(req.message,"/"));
                        strcpy(password,strtok(NULL,"/\n"));

                        printf("Group name: %s, password of group: %s\n",name,password);
                        fflush(stdout);

                        group_id = create_group(db,user_id,name,password);


                        for(int client_fd = 0 ; client_fd < client_fds_lenght + 1; client_fd++)
                        {
                            if(client_fds[client_fd].fd == client_socket_fd ) {
                                client_fds[client_fd].group_id = group_id;
                                printf("Group id %d\n",client_fds[client_fd].group_id);
                            }
                        }

                        update_users_field(db,"ADMIN",user_id,"1");
                        

                        strcpy(response_to_client,"The group was succesfully created!\n");
                        sending_response(client_socket_fd,user_id,group_id,response_to_client,"",SUCCESS);
                        fflush(stdout);
                    }
                    else if (req.gr_info.group_connection == JOIN_GROUP)
                    {
                        //get list of groups
                        if(req.join_group_status == GET_LIST)
                        {
                            char *group_list = select_table(db,"SELECT * FROM GROUPS;");
                            printf("Groups: %s", group_list);
                            sending_response(client_socket_fd,user_id,-1,group_list,"",SUCCESS);
                        }
                        //select a id_group
                        else if (req.join_group_status == SELECT_GROUP)
                        {
                            char id_group[1024];
                            char password[1024];
                            strcpy(id_group,strtok(req.message,"/"));
                            strcpy(password,strtok(NULL,"/\n"));

                            int group_exist = verify_group_exist(db,id_group,password);
                            int logged_status;

                            //Exist-login
                            if(group_exist > 0)
                            {
                                fflush(stdout);
                                //Succesfull response 
                                bzero(response_to_client,sizeof(response_to_client));
                                strcpy(response_to_client,id_group);
                                printf("Response_to_client: %s\n",response_to_client);
                                sending_response(client_socket_fd,-1,atoi(id_group),response_to_client, " ",SUCCESS);
                                group_id = atoi(id_group);

                                for(int client_fd = 0 ; client_fd < client_fds_lenght + 1; client_fd++)
                                {
                                    if(client_fds[client_fd].fd == client_socket_fd) {
                                        client_fds[client_fd].group_id = atoi(id_group);
                                        printf("Id group: %d\n",client_fds[client_fd].group_id);
                                    }
                                }
                            }
                            else 
                            {
                                //Doesn't exist
                                bzero(response_to_client,1024);
                                strcpy(response_to_client,"The group doesn't exist\n");
                                sending_response(client_socket_fd,-1,-1,response_to_client,"",FAILED);
                            }

                        }
                        //enter password
                        else if (req.join_group_status == JOIN)
                        {
                            char *response = get_group_messages(db,group_id);
                            sending_response(client_socket_fd,user_id,group_id,response,"",SUCCESS);
                        }

                        fflush(stdout);
                    }
                }
                else if (req.gr_info.group_status == IN_GROUP)
                {
                    
                    //verify if we should to return the messages from db
                    if(req.message_type == QUIT) 
                    {
                        sending_response(client_socket_fd,-1,-1," "," ",SUCCESS);
                        remove_client_fd(client_socket_fd);
                    }
                    if(req.message_type == BACK)
                    {
                        for(int client_fd = 0 ; client_fd < client_fds_lenght; client_fd++)
                        {
                            if(client_fds[client_fd].fd != client_socket_fd && client_fds[client_fd].group_id==req.group_id && client_fds[client_fd].group_id!=-1)
                            {
                                char msg[1024];
                                sprintf(msg,"%s left the chat!\n",username);
                                printf("Res:%s\n",msg);
                                strcpy(res.message, msg);
                                res.status = SUCCESS;

                                write(client_fds[client_fd].fd,&res,sizeof(res));
                            }
                        }

                        for(int client_fd = 0 ; client_fd < client_fds_lenght; client_fd++)
                        {
                            if(client_fds[client_fd].fd == client_socket_fd) {
                                client_fds[client_fd].group_id = -1;
                            }
                        }
                        // sending_response(client_socket_fd,-1,-1,"You was disconneted!","",SUCCESS);
                        
                    }
                    else if(req.message_type == TEXT_TRANSFER || req.message_type == FILE_UPLOAD)
                    {

                        if(req.message_type == FILE_UPLOAD)
                        {
                            //create a file in the server folder
                            char path[1024];
                            strcpy(path,req.message);
                            int file = open(path,O_RDONLY);
        
                            char content[1024];
                            int lenght = 0;
                            char ch;
                            while (read(file,&ch,sizeof(ch)) != 0)
                            {
                                content[lenght++] = ch;
                            };
                            content[lenght] = '\0';

                            char *file_name = strrchr(path,'/');
                            file_name += 1;
                            file_name[strlen(file_name)] = '\0';

                            //save in the db the file_name
                            save_file(db,group_id,user_id,file_name);

                            int save_file = creat(file_name,O_WRONLY);

                            for (int i = 0; i < strlen(content); i++)
                            {
                                write(save_file,&content[i],sizeof(char));
                            }

                            sprintf(res.message,"file_name:\"%s\" (#download:file_name,destination)\n",file_name);
                        }
                        else 
                        {
                            strcpy(res.message,req.message);
                        }

                        res.status=1;
                        strcpy(res.username,username);
                        save_message(db,user_id,group_id,res.message);
                        for(int client_fd = 0 ; client_fd < client_fds_lenght; client_fd++)
                        {
                            if(client_fds[client_fd].fd != client_socket_fd && client_fds[client_fd].group_id==req.group_id && client_fds[client_fd].group_id!=-1) {
                            
                                write(client_fds[client_fd].fd,&res,sizeof(res));
                            }
                        }
                    }
                    else if(req.message_type == FILE_DOWNLOAD)
                    {
                        printf("SMTH");
                        char request[1024];
                        strcpy(request,req.message);

                        char *file_name;
                        char *destination;

                        file_name = strtok(request,",");
                        destination = strtok(NULL, "\n");

                        //search the file in db
                        int file_exist = verify_file_exist(db,group_id,file_name);

                        fflush(stdout);
                        // //if i found the file, get the content and return to the client
                        if(file_exist > 0) {
                            char path[1024];
                            sprintf(path,"%s/%s",destination,file_name);
                            fflush(stdout);

                            int file = open(file_name,O_RDONLY);

                            char content[1024];
                            int lenght = 0;
                            char ch;
                            while (read(file,&ch,sizeof(ch)) != 0)
                            {
                                content[lenght++] = ch;
                            };
                            content[lenght] = '\0';
                            
                            sending_response(client_socket_fd,user_id,group_id,path,content,SUCCESS);
                        }
                        else 
                        {
                            printf("The file was not find!\n");
                            fflush(stdout);
                            sending_response(client_socket_fd,-1,-1,"The file was not find!","",FAILED);
                        }
                    }
                }
            }
        }
    }

    return NULL;
}

int main()
{
    sqlite3 *db = open_db("users.db"); 

    update_users_field(db,"STATUS",0, "0");
    update_users_field(db,"STATUS",1, "0");
    update_users_field(db,"STATUS",2, "0");

    //Setup the socket
    int server_socket_fd = setup_socket();

    printf("The server wait at port %d...\n", PORT);

    struct sockaddr_in client_address;
    int len = sizeof(struct sockaddr_in);

    pthread_t thread;
    while (server_socket_fd)
    {
        int client = accept(server_socket_fd,(struct sockaddr *)&client_address,&len);
        pthread_create(&thread,NULL,communication_manager, &client);
    }
    
    close(server_socket_fd);
    return 0;
}