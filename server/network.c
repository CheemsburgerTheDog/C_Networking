#ifndef NETWORK
#define NETWORK
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <poll.h>
#include "logging.c"
#include "s_network.c"
#include "authentication.c"
#include <pthread.h>

static Server *s_udp;
static Server *s_tcp;
static User *users;
int g_total_capacity = 1;
int g_current_capacity = 0;
pthread_mutex_t m_poll, m_passwd, m_users;

void process_msg(int connection) {
    //TODO! Ensure the connection has not been cancelled
    Message msg;
    recv(connection, &msg, sizeof(Message), 0);
    switch (msg.type) {
        case REGISTER: //REGISTER
            register_(connection, &msg);
            break;
    
        default:
            break;
    }
      
}
int InitServer(char *ip, int tport, int tn, int uport, int un, int capacity) {
    int temp;
    s_udp = (Server*) malloc(sizeof(Server));
    s_tcp = (Server*) malloc(sizeof(Server));
    g_total_capacity = capacity;
    users = (User*) malloc(sizeof(User)*g_total_capacity);
    memset(users, 0, sizeof(User)*g_total_capacity);
    memset(s_udp, 0, sizeof(Server));
    memset(s_tcp, 0, sizeof(Server));

    pthread_mutex_init(&m_poll, NULL);
    pthread_mutex_init(&m_passwd, NULL);
    pthread_mutex_init(&m_users, NULL);

    for (size_t i = 0; i < g_total_capacity; i++) {
        users[i].len = sizeof(users->addr);
    }

    s_udp->addr.sin_family = AF_INET;
    s_udp->addr.sin_addr.s_addr = htonl(INADDR_ANY);
    s_udp->addr.sin_port = htons(uport);
    temp = socket(AF_INET, SOCK_DGRAM, 0);
    s_udp->handle = temp;
    bind (s_udp->handle, (struct sockaddr *) &(s_udp->addr), sizeof(s_udp->addr));
    listen(s_udp->handle, un);

    s_tcp->addr.sin_family = AF_INET;
    s_tcp->addr.sin_addr.s_addr = htonl(INADDR_ANY);
    s_tcp->addr.sin_port = htons(tport);
    temp = socket(AF_INET, SOCK_STREAM, 0);
    s_tcp->handle = temp;
    // fcntl(s_tcp->handle, F_SETFL, fcntl(s_tcp->handle, F_GETFL, 0) | O_NONBLOCK) == -1 ) { log_info(ERROR, "TCP socket fnctl failure"); }
    bind (s_tcp->handle, (struct sockaddr *) & s_tcp->addr, sizeof(s_tcp->addr));
    listen(s_tcp->handle, tn);
}
//Begin accepting connections till total_capacity is reached. Needs to be run on a separete thread as it's only updating User array with new handles and active = true in-place. It does not recover any incoming data from the socket.
void Start() {
    int index = 0;
    while (1) {
        if (g_current_capacity == g_total_capacity ) {
            continue;
        }
        index = 0;
        while (index < g_total_capacity) {
            if (users[index].active == 0) {

                int temp_handle;
                struct sockaddr_in temp_sock;
                socklen_t temp_len;
                // users[index].handle = accept(s_tcp->handle, (struct sockaddr *) &(users[index].addr), &(users[index].len));
                temp_handle = accept(s_tcp->handle, (struct sockaddr *) &temp_sock, &temp_len);
                // pthread_mutex_lock(&m_users);
                process_msg(temp_handle);

                // g_current_capacity = g_current_capacity+1;
                // char text[] ="Witam";
                // send(users[index].handle, text, 6, 0);
                // send(users[index].handle, text, 6, 0);
                // send(users[index].handle, text, 6, 0);
                // close(users[index].handle);
            } else { index = index+1; }
        }    
    }
}

// void respond() {
//     recv
// }

#endif