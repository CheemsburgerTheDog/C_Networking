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
#include <pthread.h>

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

void *clock_(void*);
void inform_expired();


//DONE Initializes passwd file at path.
void InitPasswd(char path[]) {
    // srand(time(NULL));
    passwd = (Passwd*) malloc(sizeof(Passwd));
    pthread_mutex_init(&(passwd->mutex), NULL);
    passwd->file = fopen(path, "a+");
}

//DONE Login user by adding him to the users array. 
int login(int connection, Message *msg, int thread_id, pthread_mutex_t *m_users, User *s_users, int uo) {
    Credentials credentials;
    char line_buff[BUFF_SIZE];
    char *token;
    sscanf(msg->message, "%s %s", credentials.login, credentials.password);
    pthread_mutex_lock(&(passwd->mutex));
    rewind(passwd->file);
    while (fgets(line_buff, BUFF_SIZE, passwd->file)!=NULL) {
        token = strtok(line_buff, " ");
        if (strcmp(token, credentials.login) == 0 ) {
            token = strtok(NULL, " ");
            if (strcmp(token, credentials.password) == 0) {
                token = strtok(NULL, " ");
                credentials.type = atoi(token);
                pthread_mutex_unlock(&(passwd->mutex));
                ///////////////INSERTING USER////////////////
                pthread_mutex_lock(m_users);
                for (size_t i = 0; i < uo; i++) {
                    if ( s_users[i].active == 0) {
                        s_users[i].active = 1;
                        pthread_mutex_unlock(m_users);
                        s_users[i].addr = ;///;
                        s_users[i].len = ;///;
                        s_users[i].tid = thread_id;
                        strcpy(s_users[i].name, credentials.login);
                        s_users[i].busy = 0;
                        s_users[i].handle = connection;
                        s_users[i].timeout = 300; //Seconds
                        s_users[i].type = credentials.type;
                        Message msg;
                        sprintf(msg.message, "%d", credentials.type);
                        send_(connection, LOGIN_SUCCESFUL, &msg);
                        return 0;
                    }
                }
                ///////////// END OF INSERTING ///////////////
            }             
        }
    }
    pthread_mutex_unlock(&(passwd->mutex));
    send_(connection, LOGIN_FAILED, NULL);
    close(connection);
    return 1;
}

//DONE Register user in passwd file then close the connection.
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
                send_(connection, REGISTER_FAILED, NULL);
                close(connection);
                return 1;
        }
    }
    //  fprintf(passwd->file, "%s %s %d\n", credentials.login, credentials.password, credentials.type, new_id);
    fprintf(passwd->file, "%s %s %d\n", credentials.login, credentials.password, credentials.type); //Print new credentials into passwd
    fflush(passwd->file);
    pthread_mutex_unlock(&(passwd->mutex));
    send_(connection, REGISTER_SUCCESFUL, NULL);
    close(connection);
    return 0;
}
/* Thread-only clock function . Every one or so seconds it performs all time based operations on the ETAs. 
    Returns a message:
        BID_DECLINE with "{id}{lowETA}{supETA}" on failure
        BID_ACCEPT wtih "{id}" on successful election 
*/
void *clock_(void *str) {    
    while(1) {
        sleep(1);
        system("clear");
        for (size_t i = 0; i < ((Sclock*)str)->u_size; i++) {
            if (((Sclock*)str)->uptr[i].active == true) {
                ((Sclock*)str)->uptr[i].timeout = ((Sclock*)str)->uptr[i].timeout - 1;
            }
        }
        for (size_t i = 0; i < ((Sclock*)str)->o_size; i++) {
            if (((Sclock*)str)->optr[i].phase == 0 ) { continue; }
            ((Sclock*)str)->optr[i].active_eta = ((Sclock*)str)->optr[i].active_eta - 1;
            if (((Sclock*)str)->optr[i].active_eta < 0) {
                if ( ((Sclock*)str)->optr[i].lowSup_eta == 100000) {
                    ((Sclock*)str)->optr[i].phase = 0;
                    send_(((Sclock*)str)->optr[i].cli_handle, OFFER_TIMEOUT, NULL);
                } else {
                    Message msg;
                    sprintf(msg.message, "%d", (((Sclock*)str)->optr[i].id));
                    send_( (((Sclock*)str)->optr[i].sup_handle), TRANSACTION_STARTED, &msg );
                }
                for (size_t j = 0; j < ((Sclock*)str)->o_size; j++) {
                    if ( ((Sclock*)str)->optr[i].cli_handle == ((Sclock*)str)->uptr[j].handle ) { ((Sclock*)str)->uptr[j].busy = false; }
                }
            } else {
                //ID  ETA NAME RESOURCE QUA LOWETA
                printf("%d %d %s %s %d %d\n",
                ((Sclock*)str)->optr[i].id,
                ((Sclock*)str)->optr[i].active_eta,
                ((Sclock*)str)->optr[i].client_name,
                ((Sclock*)str)->optr[i].resource,
                ((Sclock*)str)->optr[i].quantity,
                ((Sclock*)str)->optr[i].lowSup_eta
                );
            }
        }
        fflush(stdout);
    }
}

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