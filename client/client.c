#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include "../utils/constants.h"
#include <pthread.h>

int port;
int is_connected = NOT_LOGGED;
int group_status = OUT_GROUP;
int group_id = -1;
int user_id = -1;
char username[1024];
char group_name[1024];

struct write_thread {
  char msg[100];
  int socket_fd;
};

int setup_socket()
{
  int socket_fd;
  if ((socket_fd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror ("socket() ERROR!\n");
    exit(EXIT_FAILURE);
  }

  return socket_fd;
}

void connect_to_server(int server_socket_fd, char* ip_adress, int port)
{
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(ip_adress);
  /* portul de conectare */
  server.sin_port = htons(port);
  
  /* ne conectam la server */
  if (connect (server_socket_fd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
  {
    perror ("connect() ERROR!\n");
    exit(EXIT_FAILURE);
  }
}
//read data from the server
void* read_message(void * socket_fd)
{
  char buffer[1024];
  struct response res;
  while (1)
  {
    if(read(*(int *)socket_fd,&res,sizeof(res)) <= 0)
    {
      perror("\nError: The client was disconnected!");
      exit(EXIT_FAILURE);
    }
    
    if(res.status == SUCCESS) {
      printf("\n%s: %s", res.username,res.message);
      fflush(stdout);
      printf("%s:", username);
      fflush(stdout);
    }
    else 
    {
      printf("%s\n",res.message);
    }
    pthread_testcancel();
  }
  
  pthread_exit(NULL);
}

void sending_request(int socket_fd, enum request_constants logging_status, enum group_status gr_status, enum group_connection gr_connection,int join_status, char *buffer)
{
  struct request req;
  req.user_id = user_id;
  req.group_id = group_id;
  req.logging_status = logging_status;
  req.gr_info.group_status = gr_status;
  req.gr_info.group_connection = gr_connection;
  req.join_group_status = join_status;
  strcpy(req.message, buffer);
  if(write(socket_fd,&req, sizeof(req)) < 0)
  {
    perror("WRITING ERROR!");
    exit(EXIT_FAILURE);
  }
}

struct response recieving_response(int socket_fd)
{
  struct response resp;

  if (read (socket_fd, &resp, sizeof(resp)) < 0)
  {
    perror ("read() ERROR!\n");
    exit(EXIT_FAILURE);
  }

  return resp;
}

int main (int argc, char *argv[])
{
  char msg[100];		//

  /* Connect parameters */
  if (argc != 3)
  {
    printf ("SINTAXA: %s <adresa_server> <port>\n", argv[0]);
    return -1;
  }
  
  int socket_fd = setup_socket();			

  connect_to_server(socket_fd,argv[1],atoi(argv[2]));
  
  struct request req;
  struct response res; 

  int running = 1;

  pthread_t read_thread;
  pthread_t write_thread;


  while (running)
  {
    if(is_connected == NOT_LOGGED) {

      // //Enter the email/password
      bzero (msg, 100);
      printf ("Enter your email/password\n(username/password):");
      fflush (stdout);

      int bytes;
      if((bytes = read (0, msg, 100)) < 0)
      {
        perror("READING ERROR");
        exit(EXIT_FAILURE);
      }

      //Send the username and the password 
      sending_request(socket_fd,NOT_LOGGED,OUT_GROUP,NONE,-1,msg);

      //Recieving the response from the server
      res = recieving_response(socket_fd);
      
      if(res.status == SUCCESS)
      {
        printf("\nWelcome %s!\n", res.message);
        strcpy(username,res.message);
        is_connected = LOGGED;
        user_id = res.user_id;
      }
      else 
      {
        printf("%s\n",res.message);
      }
    }
    else if(is_connected == LOGGED)
    {
        if(group_status == OUT_GROUP)
        {
          char connect_group_status[2];
          bzero(connect_group_status,sizeof(connect_group_status));

          printf("\n(0)Create group\n(1)Join group\nEnter:");
          fflush(stdout);
          if(read(0,connect_group_status,sizeof(connect_group_status))==-1)
          {
            perror("READING connect_group_status ERROR.");
          }

          char *group_connection = strtok(connect_group_status,"\n");

          if(atoi(group_connection) == CREATE_GROUP)
          {
            char group_info[100];
            printf("Enter group name/password:");
            fflush(stdout);
            
            if(read(0,group_info,sizeof(group_info))==-1)
            {
              perror("Reading group_info error");
            }


            sending_request(socket_fd,LOGGED,OUT_GROUP,CREATE_GROUP,-1,group_info);
            
            res = recieving_response(socket_fd);

            if(res.status == SUCCESS)
            {
              group_id = res.group_id;
              group_status = IN_GROUP;
            }
            else 
            {
              printf("%s\n",res.message);
            }
          }
          else if (atoi(group_connection) == JOIN_GROUP)
          {
            //Print the list of the groups
            sending_request(socket_fd,LOGGED,OUT_GROUP,JOIN_GROUP,GET_LIST,"");

            
            res = recieving_response(socket_fd);
            printf("\nLIST OF GROUPS:\n%s", res.message);
            fflush(stdout);

            //Select a group id
            char id_group[100] = " \0";
            

            while (strcmp(id_group," ") == 0)
            {
              printf("Select ID of a group: ");
              
              fflush(stdout);
              if(read(0,id_group,sizeof(id_group))==-1)
              {
                perror("Reading group_info error");
              }

              //to do 
              //verify if the id_group exist
              break;
            }

            char group_info[1024];
            id_group[strlen(id_group)-1] = '\0';


            printf("ENTER PASSWORD:");
            fflush(stdout);
            
            char password[100];
            if(read(0,password,sizeof(password))==-1)
            {
              perror("Reading group_info error");
            }
            sprintf(group_info,"%s/%s",id_group,password);

            sending_request(socket_fd,LOGGED,OUT_GROUP,JOIN_GROUP,SELECT_GROUP,group_info);
            bzero(&res,sizeof(res));
            res = recieving_response(socket_fd);

            if(res.status == SUCCESS)
            {
              printf("\nWelcome in %s group, %s!\n\n", res.message, username);
              fflush(stdout);
              sending_request(socket_fd,LOGGED,OUT_GROUP,JOIN_GROUP,JOIN,"");
              bzero(&res,sizeof(res));
              res = recieving_response(socket_fd);

              printf("%s",res.message);
              group_status = IN_GROUP;
              group_id = res.group_id;
            }
            else 
            {
              printf("%s",res.message);
            }
          }
        }
        else if(group_status == IN_GROUP) {

          pthread_create(&read_thread,NULL,read_message,&socket_fd);

          char buffer[1024];
          bzero(buffer,sizeof(buffer));
          printf("%s:", username);
          fflush(stdout);
          if(read(0,buffer,sizeof(buffer)) <= 0)
          {
            exit(EXIT_FAILURE);
          }

          if(strcmp(buffer, "#back\n") == 0)
          {
            group_status = OUT_GROUP;
            group_id = -1;
            
            sending_request(socket_fd,LOGGED,IN_GROUP,NONE,-1,buffer);
            pthread_cancel(read_thread);
          }
          else 
          {
            sending_request(socket_fd,LOGGED,IN_GROUP,NONE,-1,buffer);
          }
        }
    }
  }
  close(socket_fd);
}