#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _GNU_SOURCE
#include <sys/socket.h>
#include <logging.c>
#define BUFF_SIZE 30
#define CRED_SIZE 10
#include<pthread.h>
typedef struct {
    int id;
    char login[CRED_SIZE];
    char password[CRED_SIZE];
    int type;
} Credentials;

typedef struct {
    FILE *file;
    pthread_mutex_t mutex;
} Passwd;

void InitPasswd(Passwd passwd, char path[]) {
    pthread_mutex_init(&(passwd.mutex), NULL);
    passwd.file = fopen(path, "a+");
}
int LoginUser(int socket, Passwd *passwd) {
    Credentials user;
    user.id = 1111;
    user.type = 222;
    scanf("%s", user.login);
    scanf("%s", user.password);
    pthread_mutex_lock(&(passwd->mutex));
    rewind(passwd->file);
    char line_buff[BUFF_SIZE];
    char *token;
    while (fgets(line_buff, BUFF_SIZE, passwd->file)!=NULL) {
        token = strtok(line_buff, " ");
        if (strcmp(token, user.login) == 0 ) {
            token = strtok(NULL, " ");
            if (strcmp(token, user.password) == 0) {
                token = strtok(NULL, " ");
                user.id = atoi(token);
                token = strtok(NULL, " ");
                user.type = atoi(token);
                pthread_mutex_unlock(&(passwd->mutex));
                printf("Zalogowano %s %s %d %d", user.login, user.password, user.id, user.type);
                return 0;
            }             
               /* TODO! recv() close(socket); */
        } else { printf("Nie Znaleziono matcha"); }
    }
    pthread_mutex_unlock(&(passwd->mutex));
    return 1;
}

int RegisterUser(int socket, Passwd passwd) {
    int counter = 0;
    Credentials user;
    pthread_mutex_lock(&(passwd.mutex));
    rewind(passwd.file);
    //recv(socket, &user, sizeof(Credentials), 0)
    char line_buff[BUFF_SIZE];
    while (fgets(line_buff, BUFF_SIZE, passwd.file)!=NULL) {
        char *token;
        token = strtok(line_buff, " ");
        if (strcmp(token, user.login) == 0 ) {
                printf("Znaleziono matcha");
                pthread_mutex_unlock(&(passwd.mutex));
                return 1;
               /* TODO! recv() close(socket); */
        } else { printf("Nie Znaleziono matcha"); }
    }
    fprintf(passwd.file, "%s %s %d %d\n",user.login, user.password, user.id, user.type);
    pthread_mutex_unlock(&(passwd.mutex));
    return 0;
}
