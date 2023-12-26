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

#define NEW_OFFER 31
#define NEW_ACCEPTED 32
#define NEW_DECLINED 33
#define NEW_INPROGRESS 37
#define ACCEPT_OFFER 34
#define ACCEPT_ACCEPT 35
#define ACCEPT_DECLINE 36
#define OFFER_FINISHED 38

#define USER_TIMEOUT 50
#define OFFER_TIMEOUT 51

#define HERROR 60



typedef struct server {
    struct sockaddr_in addr;
    int handle;
    int max_cap;
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

typedef struct tinfo {
    int max;
    int id;
} Tinfo;

typedef struct message {
    int session_id;
    int code;
    char message[MSG_LEN];
} Message;

typedef struct offer {
    //0 -> open
    //1 -> in progrress
    //2 -> done
    int phase;
    int cli_handle;
    int sup_handle;
    int id;
    int eta;
    char client_name[10];
    char resource[20];
    int quantity;
} Offer;

typedef struct sclock {
    int u_size;
    User *uptr;
    int o_size;
    Offer *optr;
} Sclock;

#endif