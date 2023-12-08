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

/* codul de eroare returnat de anumite apeluri */
extern int errno;

int port;
int isConnected = 0;

struct response
{
    int status;
    char message[100];
};

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

  //Enter the email/password
  bzero (msg, 100);
  printf ("[client]Enter your email/password ('/'- delimitator):");
  fflush (stdout);
  if(read (0, msg, 100) <= 0)
  {
    perror("Reading email error");
    exit(EXIT_FAILURE);
  }
  
  //Send email/password
  if (write (socket_fd, msg, 100) <= 0)
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
  printf ("[client]The response is: %s\n", resp.message);

  isConnected = resp.status;
  if (isConnected)
  {
      printf("Yey, I'm log-in!\n");
  }
  
  close(socket_fd);
}
