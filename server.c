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

#define PORT 1025
char email[100];

struct response
{
    int status;
    char message[100];
};

char * conv_addr (struct sockaddr_in address)
{
  static char str[25];
  char port[7];

  /* adresa IP a clientului */
  strcpy (str, inet_ntoa (address.sin_addr));	
  /* portul utilizat de client */
  bzero (port, 7);
  sprintf (port, ":%d", ntohs (address.sin_port));	
  strcat (str, port);
  return (str);
}

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

int logIn(int fd, char email[100])
{
    struct response resp;
    char responseToClient[100];
    char buffer[100];
    int bytes;
    if ((bytes = read (fd, buffer, sizeof(buffer))) < 0)
    {
        perror ("The email was not read\n");
        exit(EXIT_FAILURE);
    }

    strcpy(email,buffer);

    printf ("[server]The email was received:%s", email);
        
    /*Succesfull response */
    bzero(responseToClient,100);
    strcpy(responseToClient,"Log-in successfully with email: ");
    strcat(responseToClient,email);

    //Configure the response structure
    strcpy(resp.message,responseToClient);
    resp.status=1;
        
    if (bytes && write(fd, &resp, sizeof(resp)) <0)
    {
        perror ("[server] The response was not send.\n");
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

    printf ("[SERVER] Asteptam la portul %d...\n", PORT);

    struct timeval tv;

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    while (1)
    {
        /* ajustam multimea descriptorilor activi (efectiv utilizati) */
        bcopy ((char *) &active_fds, (char *) &read_fds, sizeof (read_fds));

        /* apelul select() */
        if (select (nfds+1, &read_fds, NULL, NULL, &tv) < 0)
        {
            perror ("[server] Eroare la select().\n");
        }
        /* vedem daca e pregatit socketul pentru a-i accepta pe clienti */
        if (FD_ISSET (socket_fd, &read_fds))
        {
            /* pregatirea structurii client */
            int len = sizeof (client_addr);
            bzero (&client_addr, sizeof (client_addr));

            /* a venit un client, acceptam conexiunea */
            int client = accept (socket_fd, (struct sockaddr *) &client_addr, &len);

            /* eroare la acceptarea conexiunii de la un client */
            if (client < 0)
            {
                perror ("[server] Eroare la accept().\n");
                continue;
            }

            if (nfds < client) /* ajusteaza valoarea maximului */
                nfds = client;
                    
            /* includem in lista de descriptori activi si acest socket */
            FD_SET (client, &active_fds);

            printf("[server] S-a conectat clientul cu descriptorul %d, de la adresa %s.\n",client, conv_addr (client_addr));
            fflush (stdout);
        }
        /* vedem daca e pregatit vreun socket client pentru a trimite raspunsul */
        for (int fd = 0; fd <= nfds; fd++)	/* parcurgem multimea de descriptori */
        {
            /* este un socket de citire pregatit? */
            if (fd != socket_fd && FD_ISSET (fd, &read_fds))
            {	
                if (logIn(fd,email))
                {
                    printf ("[server] S-a deconectat clientul cu descriptorul %d.\n",fd);
                    fflush (stdout);
                    close (fd);		/* inchidem conexiunea cu clientul */
                    FD_CLR (fd, &active_fds);/* scoatem si din multime */
                }
            }
        }			/* for */
    }				/* while */
    printf("Ok!");
}