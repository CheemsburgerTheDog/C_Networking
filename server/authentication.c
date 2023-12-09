#ifndef AUTHENTICATION
#define AUTHENTICATION
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _GNU_SOURCE
#include <sys/socket.h>
#include "logging.c"
#include  "s_network.c"
#define BUFF_SIZE 30
#define CRED_SIZE 10
#include<pthread.h>
typedef struct {
    char login[CRED_SIZE];
    char password[CRED_SIZE];
    int type;
} Credentials;

typedef struct {
    FILE *file;
    pthread_mutex_t mutex;
} Passwd;

static Passwd *passwd;

void InitPasswd(char path[]) {
    srand(time(NULL));
    // int r = rand();
    passwd = (Passwd*) malloc(sizeof(Passwd));
    pthread_mutex_init(&(passwd->mutex), NULL);
    passwd->file = fopen(path, "a+");
}

int login(Message *msg) {
    Credentials user;
    char line_buff[BUFF_SIZE];
    char *token;
    int id;

    pthread_mutex_lock(&(passwd->mutex));
    rewind(passwd->file);

    while (fgets(line_buff, BUFF_SIZE, passwd->file)!=NULL) {
        token = strtok(line_buff, " ");
        if (strcmp(token, user.login) == 0 ) {
            token = strtok(NULL, " ");
            if (strcmp(token, user.password) == 0) {
                token = strtok(NULL, " ");
                id = atoi(token);
                token = strtok(NULL, " ");
                user.type = atoi(token);
                pthread_mutex_unlock(&(passwd->mutex));
                printf("Zalogowano %s %s %d %d", user.login, user.password, id, user.type);
                return 0;
            }             
               /* TODO! recv() close(socket); */
        } else { printf("Nie Znaleziono matcha"); }
    }
    pthread_mutex_unlock(&(passwd->mutex));
    return 1;
}

int register_(int connection, Message *msg) {
    int counter = 0;
    char *token;
    Credentials credentials;
    
    token = strtok(msg->message, " "); //Extract login
    strcpy(token, credentials.login);
    token = strtok(NULL, " "); //Extract password
    strcpy(token, credentials.password);
    token = strtok(NULL, " "); //Extract type
    strcpy(atoi(token), credentials.type);

    pthread_mutex_lock(&(passwd->mutex)); //Keep the mutex for now. Connections will be handled 1by1 prolly.
    rewind(passwd->file);
    char line_buff[BUFF_SIZE];
    int new_id = rand()&10000000;
    while (fgets(line_buff, BUFF_SIZE, passwd->file)!=NULL) {
        token = strtok(line_buff, " ");
        if (strcmp(token, credentials.login) == 0 ) {
                pthread_mutex_unlock(&(passwd->mutex));
                Message msg;
                msg.type = REGISTER_FAILED;
                send(connection, &msg, sizeof(Message), 0);
                close(connection);
                return 1;
               /* TODO! recv() close(connection); */
        // } else {
        //     token = strtok(NULL, "*"); TODO! POSSIBLE ID OVERLAP!!!
        }
    }
    fprintf(passwd->file, "%s %s %d*%d\n",credentials.login, credentials.password, credentials.type, new_id);
    pthread_mutex_unlock(&(passwd->mutex));
    close(connection);
    return 0;
}
#endif
