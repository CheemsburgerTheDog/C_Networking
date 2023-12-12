#ifndef S_NETWORK
#define S_NETWORK
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdbool.h>
#define CAPACITY 10
#define MSG_LEN 100
#define MSG_TYPE 10

#define NSPEC 0
#define SUPPLIER 1
#define CLIENT 2

#define REGISTER 10
#define REGISTER_SUCCESFUL 11
#define REGISTER_FAILED 12

#define LOGIN 20
#define LOGIN_SUCCESFUL 21 
#define LOGIN_FAILED 22


typedef struct server {
    struct sockaddr_in addr;
    int handle;
} Server;

typedef struct user {
    struct sockaddr_in addr;
    socklen_t len;
    int handle; //TCP ONLY
    int session_id;
    int timeout;
    int type;
    int published;
    bool active;
    bool busy; 
} User;

typedef struct message {
    int session_id;
    int code;
    char message[MSG_LEN];
} Message;
#endif