#define PORT 1025
#define CLIENT_LISTENS 10

struct client_connected {
  int socket;
  struct sockaddr_in client_adress;
};




enum connection {NOT_LOGGED,LOGGED,OUT_GROUP,IN_GROUP,LOG_OUT};

enum group {CREATE_GROUP,JOIN_GROUP};

struct communication {
  int communication_type;
  char message[100];
};

struct response
{
    int status;
    char message[100];
};