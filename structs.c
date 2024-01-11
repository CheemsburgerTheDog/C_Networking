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
#define TRASACTION_ABANDON 53
#define LIMIT_REACHED 52
#define HERROR 60

typedef struct server {
    struct sockaddr_in addr;
    int handle;
    int max_cap;
} Server;


typedef struct user {
    int active; //0 Nie 1 Niezalogowany 2 Zalogowany
    struct sockaddr_in addr; //Adres uzytkownika
    socklen_t len; //Dlugosc adresu uzytkownika
    int handle; //Deskryptor
    int type; // Typ uzytkownika
    char name[10]; // Nazwa uzytkownika
    int log_timeout; //Czas 
    int tid; //Do jakiego watku nalezy
} User;

typedef struct tinfo {
    int max;
    int id;
} Tinfo;

typedef struct message {
    int code;
    char message[100];
} Message;

typedef struct offer {
    //0 -> unused
    //1 -> vip
    //2 -> untouched
    //3 -> awaiting
    //4 -> inprogress
    int phase;
    User *cli_ptr; //Wskaznik do klienta w tablicy uzytkownikow
    User *sup_ptr; //Wskaznik do dostawcy w tablicy uzytkownikow
    int id; //ID oferty
    int start_in; // czas do rozpoczecia transakcji
    int comp_in; //Czas do zakonczenia transakcji
    int lowest; // Obecnie najniszy bid w licytacji
    char resource[20]; // LIcytowany zasob
    int quantity; // Ilosc licytowanego zasobu
} Offer;

typedef struct offer_out {
    int state; // 0 free 2 occupied
    int id;
    char name[10];
    char resource[10];
    int quanitity;
    int eta;
} Offer_out;
#endif