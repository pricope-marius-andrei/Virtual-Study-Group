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
int group_status = IN_GROUP;
char connect_group_status[2];
int room_id = -1;

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
    read(*(int *)socket_fd,&res,sizeof(res));
    printf("\nMessage from server: %s\n", res.message);
    fflush(stdout);
    printf("Enter a message:");
    fflush(stdout);
  }
  
  return NULL;
}

void sending_request(int socket_fd, int logging_status, int group_status, char *buffer)
{
  struct request req;
  req.logging_status = logging_status;
  req.group_status = group_status;
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
  int running = 1;

  pthread_t read_thread;
  pthread_t write_thread;

  while (running)
  {
    if(is_connected == NOT_LOGGED) {

      // //Enter the email/password
      bzero (msg, 100);
      printf ("[client]Enter your email/password ('/'- delimitator):");
      fflush (stdout);

      int bytes;
      if((bytes = read (0, msg, 100)) < 0)
      {
        perror("READING ERROR");
        exit(EXIT_FAILURE);
      }

      //Send the username and the password 
      sending_request(socket_fd,NOT_LOGGED,OUT_GROUP,msg);

      //Recieving the response from the server
      struct response resp = recieving_response(socket_fd);
      
      if(resp.status == SUCCESS)
      {
        printf("Welcome %s!\n", resp.message);
        is_connected = LOGGED;
      }
      else 
      {
        printf("%s\n",resp.message);
      }
    }
    else if(is_connected == LOGGED)
    {
        if(group_status == OUT_GROUP)
        {
          printf("Enter (0)Create group/(1)Join group:");
          fflush(stdout);
          if(read(0,connect_group_status,sizeof(connect_group_status))==-1)
          {
            perror("Reading connect_group_status error");
          }

          printf("connect_group_status: %d", atoi(connect_group_status));

          if(atoi(connect_group_status) == CREATE_GROUP)
          {
            char group_info[100];
            printf("Enter group name/password:");
            if(read(0,group_info,sizeof(group_info))==-1)
            {
              perror("Reading group_info error");
            }

            printf("Group with name/password: %s was succesfuly created", group_info);
            group_status = IN_GROUP;
          }
          else if (atoi(connect_group_status) == JOIN_GROUP)
          {
            char group_info[100];
            printf("Enter group id/password:");
            if(read(0,group_info,sizeof(group_info))==-1)
            {
              perror("Reading group_info error");
            }

            printf("Group with id/password: %s was succesfuly created", group_info);
            group_status = IN_GROUP;
          }
  
        }
        else if(group_status == IN_GROUP) {

          pthread_create(&read_thread,NULL,read_message,&socket_fd);

          char buffer[1024];
          bzero(buffer,sizeof(buffer));
          printf("Enter a message:");
          fflush(stdout);
          read(0,buffer,sizeof(buffer));

          sending_request(socket_fd,LOGGED,IN_GROUP,buffer);

        }
    }
  }
  close(socket_fd);
}
