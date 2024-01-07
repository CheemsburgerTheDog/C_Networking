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
#include <pthread.h>

#define BUFF_SIZE 30
#define CRED_SIZE 10

#define CAPACITY 10
#define MSG_LEN 100
#define MSG_TYPE 10

#define NSPEC 0
#define SUPPLIER 2
#define CLIENT 1

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
#define TRANSACTION_STARTED 39
#define TRANSACTION_FINISHED 38
#define INPROGRESS 40
#define BID_ACCEPT 41
#define BID_DECLINE 42
#define BID_BYE 43
#define USER_TIMEOUT 50
#define OFFER_TIMEOUT 51
#define OFFER_ABANDON 53
#define LIMIT_REACHED 52
#define HERROR 60



typedef struct server {
    struct sockaddr_in addr;
    int handle;
    int max_cap;
} Server;

typedef struct user {
    int active; //Czy struktura jest uzywana
    int logged;
    struct sockaddr_in addr; //Adres uzytkownika
    socklen_t len; //Dlugosc adresu uzytkownika
    int handle; //Deskryptor
    int type; // Typ uzytkownika
    int busy; // Czy jest zajety
    char name[10]; // Nazwa uzytkownika
    int timeout; //Czas 
    int tid; //Do jakiego watku nalezy
} User;

typedef struct tinfo {
    int max;
    int id;
} Tinfo;

typedef struct message {
    int code;
    char message[MSG_LEN];
} Message;

typedef struct offer {
    //0 -> unused
    //1 -> in progrress
    int phase;
    User *cli_ptr; //Wskaznik do klienta w tablicy uzytkownikow
    User *sup_ptr; //Wskaznik do dostawcy w tablicy uzytkownikow
    int id; //ID oferty
    int active_eta; //Czas do zakonczenia licytacji
    int lowSup_eta; // Obecnie najniszy bid w licytacji
    char resource[20]; // LIcytowany zasob
    int quantity; // Ilosc licytowanego zasobu
} Offer;

typedef struct {
    char login[CRED_SIZE];
    char password[CRED_SIZE];
    int type;
} Credentials;

typedef struct {
    FILE *file;
    pthread_mutex_t mutex;
} Passwd;

typedef struct sclock {
    int u_size;
    User *uptr;
    int o_size;
    Offer *optr;
    int *tLoad_ptr;
} Sclock;

#endif