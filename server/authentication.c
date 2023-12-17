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
#include "network.c"
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

int _send_status(int connection, int code, bool close);
void clock_();

void InitPasswd(char path[]) {
    srand(time(NULL));
    passwd = (Passwd*) malloc(sizeof(Passwd));
    pthread_mutex_init(&(passwd->mutex), NULL);
    passwd->file = fopen(path, "a+");
}
//Login user into the system
int login(int connection, Message *msg, int thread_id, pthread_mutex_t *m_users, User *s_users) {
    Credentials credentials;
    char line_buff[BUFF_SIZE];
    char *token;

    token = strtok(msg->message, " "); //Extract login
    strcpy(credentials.login, token);
    token = strtok(NULL, " "); //Extract password
    strcpy(credentials.password, token);
    // int session_id = _gen_session_id(); //Get session id;

    pthread_mutex_lock(&(passwd->mutex));
    rewind(passwd->file);
    while (fgets(line_buff, BUFF_SIZE, passwd->file)!=NULL) {
        token = strtok(line_buff, " ");
        if (strcmp(token, credentials.login) == 0 ) {
            token = strtok(NULL, " ");
            if (strcmp(token, credentials.password) == 0) { //If match found insert new user
                token = strtok(NULL, " ");
                credentials.type = atoi(token);
                pthread_mutex_unlock(&(passwd->mutex));
                ///////////////INSERTING USER////////////////
                bool done = true;
                pthread_mutex_lock(m_users);
                int i = 0;
                while (1) { //LOWER-LEVEL GUARANTEE OF FREE SPACE THOU ERROR PRONE
                    if ( s_users[i].active == false) {
                        s_users[i].active = true;
                        s_users[i].busy = false;
                        s_users[i].handle = connection;
                        s_users[i].published = 0;
                        // s_users[i].session_id = session_id;
                        s_users[i].timeout = 300; //Seconds
                        s_users[i].type = credentials.type;
                        pthread_mutex_unlock(m_users);
                        _send_status(connection, LOGIN_SUCCESFUL, false);
                        return 0;
                        break;
                    }
                    i = i+1;
                }
                ///////////// END OF INSERTING ///////////////
            }             
        }
    }
    pthread_mutex_unlock(&(passwd->mutex));
    _send_status(connection, LOGIN_FAILED, true);
    return 1;
}
//Register user in passwd
int register_(int connection, Message *msg) {
    int counter = 0;
    char *token;
    Credentials credentials;
    Message respond;
    
    token = strtok(msg->message, " "); //Extract login
    strcpy(credentials.login, token);
    token = strtok(NULL, " "); //Extract password
    strcpy(credentials.password, token);
    token = strtok(NULL, " "); //Extract type
    credentials.type = atoi(token);

    pthread_mutex_lock(&(passwd->mutex)); //Keep the mutex for now. Connections will be handled 1by1 prolly.
    rewind(passwd->file);
    char line_buff[BUFF_SIZE];
    while (fgets(line_buff, BUFF_SIZE, passwd->file)!=NULL) { //Fail if login duplicate found
        token = strtok(line_buff, " ");
        if (strcmp(token, credentials.login) == 0 ) { // REGISTER FAILED - NAME TAKEN
                pthread_mutex_unlock(&(passwd->mutex));
                _send_status(connection, REGISTER_FAILED, true);
                return 1;
        }
    }
    //  fprintf(passwd->file, "%s %s %d\n", credentials.login, credentials.password, credentials.type, new_id);
    fprintf(passwd->file, "%s %s %d\n", credentials.login, credentials.password, credentials.type); //Print new credentials into passwd
    fflush(passwd->file);
    pthread_mutex_unlock(&(passwd->mutex));
    _send_status(connection, REGISTER_SUCCESFUL, true);
    return 0;
}

//Generate unique session id based on users array
// int _gen_session_id() {
//     bool avaiable = true;
//     pthread_mutex_lock(&m_users);
//     while (1) {
//         avaiable = true;
//         int c_session_id = rand()%1000000;
//         for (int i = 0; i<g_total_capacity; i++) {
//             if ( users[i].session_id ==  c_session_id) {
//                 avaiable = false; 
//                 break; 
//             }
//         }
//         if (avaiable == true) { 
//             pthread_mutex_unlock(&m_users);
//             return c_session_id; 
//         }
//     }
// }
//Send status-only message. Used only for clarity;
int _send_status(int connection, int code, bool _close) {
    Message msg;
    msg.code = code;
    send(connection, &msg, sizeof(Message), 0);
    if (_close) {
        close(connection);
    }
    
}

// void clock() {
//     int elapsed = 0;
//     while(1) {
//         pthread_mutex_lock(&m_users);
//         for 
//         sleep(1);
//     }
// }
//Find a free spot and insert newly logged user into users array.
// void _insert_new_user(int connection, Credentials *creds, int session_id) {
//     bool done = true;
//     int i = 0;
//     pthread_mutex_lock(&m_users);
//     while (done) {
//         if ( users[i].active == false) {
//             users[i].active = true;
//             users[i].busy = false;
//             users[i].handle = connection;
//             users[i].published = 0;
//             users[i].session_id = session_id;
//             users[i].timeout = 300;
//             users[i].type = creds->type;
//         } 
//     }
    
#endif