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
#define MAX_OFFERS 6
pthread_t t_clock, t_sup;
static Offer_out* offers;
static int handle;
int f_block = 0;
int stdin_wakeup = 0;
int clock_print = 1;
int gid, gsETA;
int input_state = 0;
char gcli_name[10];
// MAIN 
void run(char *, int );
// Authentication
    void register_();
    void login_();
        void client_mode();
            void finalize(int);
        void *supplier_mode(void*);
            void process_msg(Message*);
        void *clock_(void*);
// UTILITIES 
void recv_(int handle, Message *);
void send_(int, Message*);
void perror_(const char *);
// Client tree
//Supplier tree


int main (int argc, char* argv[]) {
    run(argv[1], 7030);
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
            else { 
                pthread_create(&t_sup, NULL, supplier_mode, NULL);
                pthread_join(t_sup, NULL);
            }
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
        system("clear");
        fcntl(handle, F_SETFL, f_block);
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
                        finalize(eta);
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

void finalize(int internalETA) {
    int recv_eta, id;
    char sup_name[10];
    int begin = 0;
    int n = 0;
    system("clear");
    fcntl(handle, F_SETFL, fcntl(handle, F_GETFL, 0) | O_NONBLOCK);
    while(1) {
        Message temp;
        system("clear");
        if (internalETA>=0 && begin == 0 ) { printf("Offer successfully posted!\nExpires in %d\n", internalETA); } 
        else { printf("Order expired. Awaiting server comfirmation...\n"); }
        n = recv(handle, &temp, sizeof(Message), 0);
        //////////////////
        if ( n > 0 && temp.code == TRANSACTION_STARTED ) {
            sscanf(temp.message, "%d %d %s", &id, &recv_eta, sup_name);
            begin = 1; 
        }
        if ( begin == 1 && recv_eta >=0) {
            system("clear");
            printf("Supplier %s with the best eta has been chosen.\nEstimated completion time: %d\n", sup_name, recv_eta);
            recv_eta = recv_eta -1;
        }
        if ( begin == 1 && recv_eta < 0 ) {
            recv_eta = recv_eta -1;
            system("clear");
            printf("Awaiting server response. Delay: %d\n", (-1)*recv_eta);
        }
        /////////////////////
        if (n > 0 && temp.code == TRANSACTION_FINISHED) { 
            system("clear");
            printf("Supplier has completed your order. You may post a new order.\n");
            sleep(3);
            system("clear");
            return;
        }
        //////////////////////
        if (n > 0 && temp.code == OFFER_TIMEOUT) {
            system("clear");
            printf("No supplier has chosen your order, thus the order has been cancelled.\nYou may post a new order.\n");
            sleep(3);
            system("clear");
            return;
        }
        if (n > 0 && temp.code == TRASACTION_ABANDON) {
            system("clear");
            printf("No respond from the supplier. Transaction may have been abandoned.\n Post a new order.\n");
            sleep(3);
            system("clear");
            return;     
        }
        //////////////
        if ( ( n == -1 ) && ( ( begin == 0 && internalETA + 10 < 0 ) || (begin == 1 && recv_eta + 20 < 0 )) ) {
            printf("! No respond from the server !\n");
            fflush(stdout);
            sleep(2);
            system("clear");
            return;
        }
        fflush(stdout);
        internalETA = internalETA-1;
        sleep(1);
    }
    return;
}

void *supplier_mode(void *) {
    offers = (Offer_out*) malloc(sizeof(Offer_out)*MAX_OFFERS);
    memset(offers, 0, sizeof(Offer_out)*MAX_OFFERS);
    pthread_create(&t_clock, NULL, clock_, NULL);
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
                char buff[20];
                n = read(0, buff, 20);
                buff[n] = '\0';
                switch (input_state) {
                    case 0:
                        int c = atoi(buff);
                        if (c == 1) { input_state = 1; }
                        if (c == 2) { 
                            close(handle);
                            exit(0);
                        }
                        break;
                    case 1:
                        Message temp;
                        strcpy(temp.message, buff);
                        temp.code = ACCEPT_OFFER;
                        send_(handle, &temp);
                        input_state = 2;
                        break;
                    case 2: { break; }
                    case 3: {
                        if (strcmp(buff, "DONE\n") == 0) {
                            Message temp;
                            temp.code = TRANSACTION_FINISHED;
                            send_(handle, &temp);
                            input_state = 0;
                            clock_print = 1;
                        }
                        break;
                    }
                    default:
                        break;
                }
            } 
            if (events[n].data.fd == handle && events[n].events == 1){
                Message received;
                recv_(handle, &received);
                process_msg(&received);
            } else if (events[n].data.fd == handle && events[n].events != 1) {
                perror_(" ! CONNECTION LOST !\n! HOST CLOSED REMOTLY !");
            }
            
        }
    }
    return;
}
void process_msg(Message *msg) {
    switch (msg->code) {
        case USER_TIMEOUT: {
            clock_print = 0;
            system("clear");
            printf("You have been disconnected from the server due to inactivity.");
            close(handle);
            exit(0);
            break;
        }
        case NEW_OFFER: {
            for (size_t i = 0; i < MAX_OFFERS; i++) {
                if (offers[i].state == 0) {
                    offers[i].state = 1;
                    sscanf(msg->message, "%d %s %d %s %d", 
                        &offers[i].id,
                        offers[i].name,
                        &offers[i].eta,
                        offers[i].resource,
                        &offers[i].quanitity);
                    offers[i].state = 2;
                    break;
                }
            }
            break;
        }
        case BID_BYE: {
            clock_print = 0;
            system("clear");
            int id, uETA, tETA;
            sscanf(msg->message, "%d %d %d", &id, &uETA, &tETA);
            printf("Your offer has been bettered.\nYour current: %d\nNew best:%d\n",uETA, tETA);
            sleep(3);
            clock_print = 1;
            input_state = 0;
            break;
        }
        case BID_DECLINE: {
            clock_print = 0;
            system("clear");
            int id, uETA, tETA;
            sscanf(msg->message, "%d %d %d", &id, &tETA, &uETA);
            printf("Offer %d denied. Better offer was proposed.\nYour ETA: %d\nCurrent lowest server ETA:%d\n", id, uETA, tETA);
            sleep(3);
            input_state = 0;
            clock_print = 1;
            break;
        } 
        case BID_ACCEPT: {
            clock_print = 0;
            system("clear");
            printf("Offer accepted. Your offer is currently the best.\n");
            break;
        }
        case TRANSACTION_STARTED: {
            sscanf(msg->message, "%d %d %s", &gid, &gsETA, gcli_name);
            clock_print = 2;
            input_state = 3;
        }
        default:
            break;
    }
}

void *clock_(void* arg) {
    while (1) {
        sleep(1);
        if (clock_print == 1) {
            system("clear");
            if (input_state == 0) {
                printf("1. Post a new bid\n2. Exit\nID  NAME  RES  AMT  ST_IN\n");    
            } else if (input_state == 1) {
                printf("FORMAT: {ID} {ETA}\n# ");
            }
            
            for (size_t i = 0; i < 10; i++) {
                if (offers[i].state !=2) { continue; }
                offers[i].eta = offers[i].eta-1;
                if (offers[i].eta <=0) { offers[i].state = 0; }
                if (offers[i].state == 2 && input_state != 1){
                    printf("%d %s %s %d %d\n",
                    offers[i].id,
                    offers[i].name,
                    offers[i].resource,
                    offers[i].quanitity,
                    offers[i].eta);  
                }
            }
        }
        if (clock_print == 2) {
            system("clear");
            printf("Transaction started.\nType DONE to end the trasaction.\nRemaining time: %d\n", gsETA);
            gsETA = gsETA -1;
        }
    fflush(stdout);
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