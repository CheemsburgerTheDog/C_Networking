#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <sys/epoll.h>
#include "/home/cheemsburger/Desktop/C_Networking/server/s_network.c"
#define MAXLIST 5

typedef struct offer_sup {
    int state; // 0 free 1 vip 2 occupied
    int id;
    char name[10];
    char resource[10];
    int quanitity;
    int eta;
} Offer_out;

typedef struct input_state {
    int state;
    int id;
    int eta;
} InputState;
static pthread_t t_clock;
InputState inputState;
int elected = 0;
int clock_print = 1;
static Offer_out* offers;
static int handle;
static char *username;
static int f_nonblock = 0;
// "127.0.0.1 7777 UDP"
void run(char *, int );
void recv_(int handle, Message *);
void send_(int, Message*);
void login_();
void register_();
void perror_(const char *);
void supplier_mode();
void client_mode();
void await_finalize(int, int);
void process_input();
void process_msg();
void *clock_();

int main (int argc, char* argv[]) {
    run("127.0.0.1", 7030);
}
void login_() {
    Message msg;
    msg.code = LOGIN;
    char login[10];
    char passwd[10];
    system("clear");
    printf("Podaj login: ");
    scanf("%s", login);
    printf("\n");
    printf("Podaj haslo: ");
    scanf("%s", passwd);
    printf("\n");
    strcpy(username, login);
    sprintf(msg.message, "%s %s", login, passwd);
    send(handle, &msg, sizeof(Message), 0);
    recv_(handle, &msg);
    switch (msg.code) {
        case LOGIN_SUCCESFUL:
            if (strcmp("1", msg.message) == 0) {
                client_mode(handle);
            } else { supplier_mode(); }
            break;
        case LOGIN_FAILED:
            perror_("Login failed");
    }
    close(handle);
}
void run(char *ip, int port) {
    int choice = 0;
    struct sockaddr_in saddr;
    username = (char*) malloc(sizeof(char)*10);

    memset(&saddr, 0, sizeof (saddr));
    saddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &(saddr.sin_addr));
    saddr.sin_port = htons(port);
    handle = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(handle, (struct sockaddr*) &saddr, sizeof(saddr)) == -1) { perror_("! Connection failed !"); }
    f_nonblock = fcntl(handle, F_GETFL, 0);
    printf("Welcome to our platform. Please login or register:\n1. Login\n2. Register\n# ");
    scanf("%d", &choice);
    switch (choice) {
        case 1:
            login_();
            break;
        case 2:
            register_();
            break;
        default:
            perror_("Wrong option");
            break;
    }
    close(handle);
}
void register_() {
    Message msg;
    msg.code = REGISTER;
    char login[10];
    char passwd[10];
    int type;
    system("clear");
    printf("Enter login: ");
    scanf("%s", login);
    printf("\n");
    printf("Enter password: ");
    scanf("%s", passwd);
    printf("\n");
    printf("Choose account type.\n1. Client\n2. Supplier\n# ");
    scanf("%d", &type);
    printf("\n");
    sprintf(msg.message, "%s %s %d", login, passwd, type);
    send_(handle, &msg);
    recv_(handle, &msg);
    switch (msg.code) {
        case REGISTER_SUCCESFUL:
            printf("Registration succesful. Reenter the app to login.\n");
            exit(0);
            break;
        case REGISTER_FAILED:
            perror_("Registration failed. Invalid format or credentials taken\n");
    }

}
void client_mode() {
    while (1) {
        fcntl(handle, F_SETFL, f_nonblock);
        int choice = 0;
        printf("1. Post new offer\n2. Exit\n# ");
        scanf("%d", &choice);
        switch (choice) {
            case 1: {
                char resource[10];
                int quanitity = 0;
                int eta = 0;
                Message msg;
                system("clear");

                printf("Enter resource: ");
                scanf("%s", resource);
                printf("\n");
                printf("Enter quantity: ");
                scanf("%d", &quanitity);
                printf("\n");
                printf("Enter ETA: ");
                scanf("%d", &eta);

                msg.code = NEW_OFFER;
                sprintf(msg.message, "%s %d %s %d", username, eta, resource, quanitity);
                send_(handle, &msg);
                recv_(handle, &msg);
                switch (msg.code) {
                    case NEW_ACCEPTED:
                        await_finalize(handle, eta);
                    case NEW_DECLINED:
                        printf("Offer not accapted. Try again\n");
                        break;
                }
                break;
            }
            case 2: {
                close(handle);
                exit(1);
                break;
            }
        }
    }
}
void supplier_mode() {
    offers = (Offer_out*) malloc(sizeof(Offer_out)*MAXLIST);
    memset(offers, 0, sizeof(Offer_out)*MAXLIST);
    pthread_create(&t_clock, NULL, clock_, NULL);
    int epollfd, nfds;
    struct epoll_event ev, events[2];
    epollfd = epoll_create(2);
    if (epollfd == -1) { perror_("EPOLL FAILURE"); }
    ev.events = EPOLLIN;
    ev.data.fd = 0;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, 0, &ev) == -1) { perror_("EPOLL_CTL ERROR"); }
    ev.data.fd = handle;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, handle, &ev) == -1) { perror_("EPOLL_CTL ERROR"); }
    while(1) {
        nfds = epoll_wait(epollfd, events, 2, -1);
        if (nfds == -1) { perror_("EPOLL ERROR"); }
        for ( int n = 0; n<2;n++ ) {
            if (events[n].data.fd == 0) { process_input(); } 
            else { process_msg(); }
        }
    }
    return;
}
void process_msg() {
    Message msg;
    recv_(handle, &msg);
    switch (msg.code) {
        case USER_TIMEOUT: {
            clock_print = 0;
            system("clear");
            printf("You have been disconnected from the server due to inactivity.");
            close(handle);
            exit(0);
            break;
        }
        case NEW_OFFER: {
            for (size_t i = 0; i < MAXLIST; i++) {
                if (offers[i].state == 0) {
                    offers[i].state = 1;
                    sscanf(msg.message, "%d %s %d %s %d", 
                        &offers[i].id,
                        offers[i].name,
                        &offers[i].eta,
                        offers[i].resource,
                        &offers[i].quanitity);
                    offers[i].state = 2;
                }
            }
            break;
        }
        case INPROGRESS: {
            system("clear");
            printf("You have applied for an offer.\nYour account is locked until order completes.\n");
            sleep(1);
            sscanf(msg.message, "%d", &inputState.eta);
            inputState.state = 4;
            break;
        }
        case BID_DECLINE: {
            system("clear");
            int id, uETA, tETA;
            sscanf(msg.message, "%d %d %d", &id, &tETA, &uETA);
            printf("Offer %d denied. Better offer was proposed.\nYour ETA: %d\nCurrent lowest server ETA:%d\n", id, uETA, tETA);
            inputState.state = 0;
            break;
        } 
        case BID_ACCEPT: {
            system("clear");
            printf("Offer accepted. Your offer is currently the best\n");
            inputState.state = 4;
            break;
        }
        default:
            break;
    }
}
void process_input() {
    int b = 0;
    char buffer[10];
    switch (inputState.state) { 
        case 0: {
            read(0, buffer, 1);
            buffer[2] = '\0';
            system("clear");
            if ( strcmp(buffer, "1") == 0 ) {
                printf("\nEnter offer ID: ");
                inputState.state = 1;
                clock_print = 0;
                return;
            }
            if (strcmp(buffer, "2") == 0) {
                close(handle);
                exit(0);
            }
            printf("\nWrong option. Try again.");
            while (read(0, buffer, 10) > 0) { continue; }
            break;
        }
        case 1: {
            b = read(0, buffer, 9);
            buffer[b] = '\0';
            int temp;
            if (temp = atoi(buffer) == 0) {
                while (read(0, buffer, 10) > 0) { continue; }
                printf("\nIncorrect ID. Try again.");
                return;
            }
            inputState.id = temp;
            inputState.state = 2;
            printf("\nEnter ETA: ");
            while (read(0, buffer, 10) > 0) { continue; }
            break;
        }
        case 2:{
            b = read(0, buffer, 9);
            buffer[b] = '\0';
            int temp;
            if (temp = atoi(buffer) == 0) {
                while (read(0, buffer, 10) > 0) { continue; }
                printf("\nIncorrect ETA. Try again.");
                return;
            }
            inputState.eta = temp;
            Message msg;
            msg.code = ACCEPT_OFFER;
            sprintf(msg.message, "%d %d", inputState.id, inputState.eta);
            send_(handle, &msg);
            inputState.state = 3;
            while (read(0, buffer, 10) > 0) { continue; }
            break;
        }
        case 3: {
            while (read(0, buffer, 10) > 0) { continue; }
            system("clear");
            printf("Awaiting server respond. Please stand by...");
            break;
        }
        case 4: {
            while (read(0, buffer, 10) > 0) { continue; }
            system("clear");
            printf("Server deems you busy. Server ETA: %d\n", inputState.eta);
            break;
        }   
    }
}
void *clock_() {
    while(1) {
        sleep(1);
        if ( clock_print == 1 ) {
            system("clear");
            printf("1. Accept offer\n2.Exit\n\n");
        }
        for (size_t i = 0; i < MAXLIST; i++) {
            if ( offers[i].state == 0 ){ continue; }
            if ( clock_print == 1 && offers[i].state == 2) {
                printf("%d %d %s %s %d\n",
                offers[i].id,
                offers[i].eta,
                offers[i].name,
                offers[i].resource,
                offers[i].quanitity
                );
            }
            offers[i].eta =  offers[i].eta-1;    
            if ( offers[i].eta < 0 ) { offers[i].state = 0; }
        }
    }
}

void recv_(int handle, Message *msg){
    if (recv(handle, msg, sizeof(Message), 0) == -1) {
        perror_("\n! Connection lost !\n");
        exit(1);
    }
}

void send_(int handle, Message *msg){
    if (send(handle, msg, sizeof(Message), 0) == -1) {
        perror_("\n! Connection lost !\n");
        exit(1);
    }
}

inline void perror_(const char *text) {
    if (handle > 1) { close(handle); }
    printf("%s", text);
    exit(1);
}
void await_finalize( int connection, int eta ) {
    Message msg;
    char name[10];
    int begin = 0 ;
    int n = 0;
    system("clear");
    fcntl(connection, F_SETFL, fcntl(connection, F_GETFL, 0) | O_NONBLOCK);
    while(1) {
        system("clear");
        if (eta>=0 && begin == 0 ) { printf("Offer successfully posted!\nExpires in %d\n", eta + 8); } 
        else { printf("Order expired. Awaiting server comfirmation...\n"); }
        n = recv(connection, &msg, sizeof(Message), 0);
        //////////////////
        if ( n > 0 && msg.code == TRANSACTION_STARTED ) {
            sscanf(msg.message, "%s %d", name, &eta);
            begin = 1; 
        }
        if ( begin == 1 ) {
            system("clear");
            printf("Supplier %s with the best ETA has been chosen. Estimated time: %d\n", name, eta);
        }
        /////////////////////
        if (n > 0 && msg.code == OFFER_FINISHED) { 
            system("clear");
            printf("Supplier has completed your order. You may post a new order.\n");
            sleep(3);
        }
        //////////////////////
        if (n > 0 && msg.code ==  OFFER_TIMEOUT) {
            system("clear");
            printf("No supplier has chosen your order, thus the order has been cancelled.\nYou may post a new order.\n");
            sleep(3);
        }
        //////////////
        if ( ( n == -1 ) && ( eta + 8 < 0 ) ) {
            printf("! No respond from the server !\n");
            fflush(stdout);
            sleep(2);
            system("clear");
            return;
        }
        fflush(stdout);
        eta = eta-1;
        sleep(1);
    }
    return;
}