#define PORT 1025

enum connection {NOT_LOGGED,LOGGED};

struct communication {
  int communication_type;
  char message[100];
};

struct response
{
    int status;
    char message[100];
};