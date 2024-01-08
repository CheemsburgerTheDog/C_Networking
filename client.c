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
#include "structs.c"

pthread_t t_clock;
static Offer_out* offers;
static int handle;
int f_block = 0;
int stdin_wakeup = 0;
// MAIN 
void run(char *, int );
// UTILITIES 
void recv_(int handle, Message *);
void send_(int, Message*);
void perror_(const char *);
// Authentication
void login_();
void register_();
// Client tree
void client_mode();
//Supplier tree
void supplier_mode();
void process_msg(Message*);


int main (int argc, char* argv[]) {
    run("10.0.0.20", 7030);
}
void run(char *ip, int port) {
    int choice = 0;
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof (saddr));
    saddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &(saddr.sin_addr));
    saddr.sin_port = htons(port);
    handle = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(handle, (struct sockaddr*) &saddr, sizeof(saddr)) == -1) { perror_("! Connection failed !"); }
    f_block = fcntl(handle, F_GETFL, 0);
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
    sprintf(msg.message, "%s %s", login, passwd);
    send_(handle, &msg);
    recv_(handle, &msg);
    switch (msg.code) {
        case LOGIN_SUCCESFUL:
            if (strcmp("1", msg.message) == 0) { client_mode(handle); } 
            else { supplier_mode(); }
            break;
        case LOGIN_FAILED:
            perror_("Login failed\n");
    }
    close(handle);
    return;
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
        default:
        case REGISTER_FAILED: {
            perror_("Registration failed. Invalid format or credentials taken\n");
        }
    }
}

void client_mode() {
    while (1) {
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
                printf("\nEnter quantity: ");
                scanf("%d", &quanitity);
                printf("\nEnter ETA: ");
                scanf("%d", &eta);
                msg.code = NEW_OFFER;
                sprintf(msg.message, "%d %s %d", eta, resource, quanitity);
                send_(handle, &msg);
                recv_(handle, &msg);
                switch (msg.code) {
                    case NEW_ACCEPTED:
                        printf("Dziala");
                        // sleep(5);
                        // // await_finalize(handle, eta);
                        // close(handle);
                        break;
                    default:
                    case LIMIT_REACHED:
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
    offers = (Offer_out*) malloc(sizeof(Offer_out)*10);
    memset(offers, 0, sizeof(Offer_out)*10);
    // pthread_create(&t_clock, NULL, clock_, NULL);
    int epollfd, nfds;
    struct epoll_event ev, events[2];
    epollfd = epoll_create(1);
    if (epollfd == -1) { perror_("EPOLL FAILURE"); }
    ev.events = EPOLLIN | EPOLLRDHUP;
    ev.data.fd = 0;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, 0, &ev) == -1) { perror_("EPOLL_CTL ERROR"); }
    ev.data.fd = handle;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, handle, &ev) == -1) { perror_("EPOLL_CTL ERROR"); }
    while(1) {
        nfds = epoll_wait(epollfd, events, 1, -1);
        if (nfds == -1) { perror_("EPOLL ERROR"); }
        for ( int n = 0; n<nfds ;n++ ) {
            if (events[n].data.fd == 0) {
                stdin_wakeup = 1; 
                char buff[15];
                n = read(0, buff, 15);
                buff[n] = '\0';
                
             } 
            if (events[n].data.fd == handle && events[n].events == 1){
                Message received;
                recv_(handle, &received);
                process_msg(&received);
            } else if (events[n].data.fd == handle && events[n].events == 1) {
                perror_(" ! CONNECTION LOST !\n! HOST CLOSED REMOTLY !");
            }
            
        }
    }
    return;
}
void process_msg(Message *msg) {
    switch (msg->code) {
        case USER_TIMEOUT: {
            system("clear");
            printf("You have been disconnected from the server due to inactivity.");
            close(handle);
            exit(0);
            break;
        }
        case NEW_OFFER: {
            for (size_t i = 0; i < 10; i++) {
                if (offers[i].state == 0) {
                    offers[i].state = 1;
                    sscanf(msg->message, "%d %s %d %s %d", 
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
        // case INPROGRESS: {
        //     system("clear");
        //     printf("You have applied for an offer.\nYour account is locked until order completes.\n");
        //     sleep(1);
        //     sscanf(msg.message, "%d", &inputState.eta);
        //     inputState.state = 4;
        //     break;
        // }
        case BID_BYE: {
            system("clear");
            int tETA;
            sscanf(msg->message, "%d", &tETA);
            printf("Your offer has been bettered.\nNew best:%d\n", tETA);
            break;
        }
        case BID_DECLINE: {
            system("clear");
            int id, uETA, tETA;
            sscanf(msg->message, "%d %d %d", &id, &tETA, &uETA);
            printf("Offer %d denied. Better offer was proposed.\nYour ETA: %d\nCurrent lowest server ETA:%d\n", id, uETA, tETA);
            break;
        } 
        case BID_ACCEPT: {
            system("clear");
            printf("Offer accepted. Your offer is currently the best\n");
            break;
        }
        default:
            break;
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
void perror_(const char *text) {
    if (handle > 1) { close(handle); }
    printf("%s", text);
    exit(1);
}