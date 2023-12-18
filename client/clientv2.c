#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include "/home/cheemsburger/Desktop/C_Networking/server/s_network.c"
static int handle;
static char *username;
// "127.0.0.1 7777 UDP"
void run(char *ip, int port);
void recv_(int handle, Message *msg);
void login_(int handle);
void register_(int handle);
void perror_(const char *text);
void supplier_mode();
void client_mode();

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
    sprintf(msg.message, "%s %s", login, passwd);
    send(handle, &msg, sizeof(Message), 0);
    recv_(handle, &msg);
    switch (msg.code) {
        case LOGIN_SUCCESFUL:
            if (strcmp("1", msg.message) == 0) {
                client_mode();
            } else { supplier_mode(); }
            break;
        case LOGIN_FAILED:
            perror_("Login failed");
    }
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
void client_mode() {
    return;
}
void supplier_mode() {
    return;
}