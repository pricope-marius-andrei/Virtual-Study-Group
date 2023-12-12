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

/* codul de eroare returnat de anumite apeluri */
extern int errno;

int port;
int is_connected = NOT_LOGGED;
pthread_mutex_t lock; 
struct communication data;

//read data from the server
void* read_message(void * socket_fd)
{
  pthread_mutex_lock(&lock);
  int bytes = read(*(int *)socket_fd,&data,sizeof(data));
 
  if(bytes == -1)
  {
    perror("Reading error");
    exit(EXIT_FAILURE);
  }
  if(bytes) {
    printf("USER:%s\n", data.message);
  }
  pthread_mutex_unlock(&lock);

  return NULL;
}

int main (int argc, char *argv[])
{
  int socket_fd;			
  struct sockaddr_in server;	// structura folosita pentru conectare 
  char msg[100];		// mesajul trimis

  /* Connect parameters */
  if (argc != 3)
  {
    printf ("[client] Sintaxa: %s <adresa_server> <port>\n", argv[0]);
    return -1;
  }

  /* Set port */
  port = atoi (argv[2]);

  /* cream socketul */
  if ((socket_fd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[client] Eroare la socket().\n");
      return errno;
    }
  

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* portul de conectare */
  server.sin_port = htons(port);
  
  /* ne conectam la server */
  if (connect (socket_fd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
  {
    perror ("[client]Eroare la connect().\n");
    return errno;
  }
  
  struct communication data;
  int running = 1;

  pthread_t th;

  while (running)
  {
    if(is_connected == NOT_LOGGED) {
      //Enter the email/password
      bzero (msg, 100);
      printf ("[client]Enter your email/password ('/'- delimitator):");
      fflush (stdout);
      if(read (0, msg, 100) <= 0)
      {
        perror("Reading email error");
        exit(EXIT_FAILURE);
      }
      
      data.communication_type = NOT_LOGGED;
      strcpy(data.message,msg);

      //Send email/password
      if (write (socket_fd, &data, sizeof(data)) <= 0)
      {
        perror ("[client]Eroare la write() spre server.\n");
        return errno;
      }

      struct response resp;
      //read the server answer
      if (read (socket_fd, &resp, sizeof(resp)) < 0)
      {
        perror ("[client]Eroare la read() de la server.\n");
        return errno;
      }
      /* afisam mesajul primit */
      printf ("%s\n", resp.message);

      is_connected = resp.status;
    }
    else if(is_connected == LOGGED)
    {
        pthread_create(&th,NULL,read_message,&socket_fd);

        bzero (msg, 100);
        // printf ("[client]Enter a message:");
        // fflush (stdout);
        if(read (0, msg, 100) <= 0)
        {
          perror("Reading message error");
          exit(EXIT_FAILURE);
        }

        if(strcmp(msg,"log-out\n")==0)
        {
          printf("log-out");
          strcpy(data.message,msg);
          data.communication_type=LOG_OUT;
          is_connected=NOT_LOGGED;
          write(socket_fd,&data,sizeof(data));
          exit(EXIT_SUCCESS);
        }
        else 
        {
          strcpy(data.message,msg);
          data.communication_type = LOGGED;
          write(socket_fd,&data,sizeof(data));
        }

        pthread_join(th,NULL);
    }
  }
  close(socket_fd);
}
