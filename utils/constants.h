#define PORT 1025
#define CLIENT_LISTENS 10

struct client_connected {
  int server_socket;
  int client_socket;
  struct sockaddr_in client_adress;
};




enum connection {NOT_LOGGED,LOGGED,OUT_GROUP,IN_GROUP,LOG_OUT};

enum group {CREATE_GROUP,JOIN_GROUP};

struct request {
  int logging_status;
  int group_status;
  char message[1024];
};

struct response
{
    int status;
    char message[1024];
};