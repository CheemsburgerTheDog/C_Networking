#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include "/home/cheemsburger/Desktop/C_Networking/server/s_network.c"
static int handle;
static char *username;
static int f_nonblock = 0;
// "127.0.0.1 7777 UDP"
void run(char *ip, int port);
void recv_(int handle, Message *msg);
void login_(int handle);
void register_(int handle);
void perror_(const char *text);
void supplier_mode();
void client_mode();
void await_finalize();

int main (int argc, char* argv[]) {
    run("127.0.0.1", 7030);
}
void login_(int handle) {
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
            login_(handle);
            break;
        case 2:
            register_(handle);
            break;
        default:
            perror_("Wrong option");
            break;
    }
    close(handle);
}
void register_(int handle) {
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
    send(handle, &msg, sizeof(Message), 0);
    recv_(handle, &msg);
    switch (msg.code) {
        case REGISTER_SUCCESFUL:
            printf("Registration succesful. Reenter the app to login.");
            exit(0);
            break;
        case REGISTER_FAILED:
            perror_("Registration failed. Invalid format or credentials taken");
    }

}
void client_mode(int handle) {
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
                    case NEW_INPROGRESS:
                        await_finalize(handle, eta);
                        break;
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
    return;
}
void recv_(int handle, Message *msg){
    if (recv(handle, msg, sizeof(Message), 0) == -1) {
        perror_("! Connection lost !");
        exit(1);
    }
}
void send_(int handle, Message *msg){
    if (send(handle, msg, sizeof(Message), 0) == -1) {
        perror_("! Connection lost !");
        exit(1);
    }
}

inline void perror_(const char *text) {
    if (handle > 1) { close(handle); }
    printf("%s", text);
    exit(1);
}
void await_finalize(int connection, int eta) {
    Message msg;
    int n = 0;
    system("clear");
    fcntl(connection, F_SETFL, fcntl(connection, F_GETFL, 0) | O_NONBLOCK);
    while(1) {
        system("clear");
        if (eta>=0) { printf("Offer successfully posted!\nExpires in %d\n", eta); } 
        else { printf("Order expired. Awaiting server comfirmation...\n"); }
        n = recv(connection, &msg, sizeof(Message), 0);
        if (n > 0 && msg.code == OFFER_FINISHED) { 
            system("clear");
            printf("Supplier has completed your order. You may post a new order.\n");
            sleep(3);
            return;
        }
        if (n > 0 && msg.code ==  OFFER_TIMEOUT) {
            system("clear");
            printf("No supplier has chosen your order, thus the order has been cancelled.\nYou may post a new order.\n");
            sleep(3);
            return;
        }
        if ( ( n == -1 ) && ( eta + 4 < 0 ) ) {
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