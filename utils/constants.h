#define PORT 1025
#define CLIENT_LISTENS 10

struct client_connected {
  int server_socket;
  int client_socket;
  struct sockaddr_in client_adress;
};

enum response_status {FAILED,SUCCESS};

enum group_connection {CREATE_GROUP,JOIN_GROUP,NONE};
enum group_status {OUT_GROUP,IN_GROUP};

enum request_constants {NOT_LOGGED,LOGGED,LOG_OUT};

struct group_info {
  int group_status; //request_constants 
  int group_connection; //group constants
};

struct request {
  struct group_info gr_info;
  int logging_status;
  int group_id;
  int user_id;
  int join_group_status;
  char message[1024];
};

struct response
{
    int status;
    int user_id; //to set user_id in the client side
    int group_id;
    char message[1024];
};

enum join_group {GET_LIST,SELECT_GROUP,JOIN};

struct client 
{
  int group_id;
  int fd;
};