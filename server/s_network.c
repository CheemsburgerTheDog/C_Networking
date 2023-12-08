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
#define MSG_LEN 50

typedef struct server {
    struct sockaddr_in addr;
    socklen_t len;
    int handle;
} Server;

typedef struct user {
    struct sockaddr_in addr;
    socklen_t len;
    int handle; //TCP ONLY
    int id;
    int type;
    int published;
    bool active;
    bool busy;
    int timeout
} User;

typedef struct message {
    char message[MSG_LEN];
} Message;
#endif