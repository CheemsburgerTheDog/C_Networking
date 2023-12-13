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
#include <sys/epoll.h>

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
    switch (msg.code) {
        case REGISTER: //REGISTER
            register_(connection, &msg);
            break;
        case LOGIN: //LOGIN
            login(connection, &msg);
            break;
        default:
            break;
    }
      
}
int InitServer(char *ip, int tport, int tn, int uport, int un, int threads, int capacity) {
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
    fcntl(s_tcp->handle, F_SETFL, fcntl(s_tcp->handle, F_GETFL, 0) | O_NONBLOCK);
    bind (s_tcp->handle, (struct sockaddr *) & s_tcp->addr, sizeof(s_tcp->addr));
    listen(s_tcp->handle, tn);

    //Calculate thread maximum load;
    int modulo  = capacity%threads;
    int max_cap = (capacity-modulo)/threads;
    //TODO SPAWN WITH max_cap;
    while (modulo>0) {
        //spawn(worker, maxcap+1)
        modulo = modulo -1;
    }
    

}
//Worker thread for handling connections. 
void worker(int listener, int max_cap) {
    struct sockaddr_in addr;
    socklen_t len;
    struct epoll_event ev, events[max_cap];
    int current_capacity = 0, connection, nfds, epollfd;
    epollfd = epoll_create(max_cap);
    // if (epollfd == -1) { VIP WITH LOGGING
    //     perror("epoll_create1");
     //     exit(EXIT_FAILURE);
    // }
    ev.events = EPOLLIN;
    ev.data.fd = listener;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listener, &ev) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }
    while (true) { // Posibble change: based on global value; SIGKILL changes the value tho false
        if (current_capacity == max_cap) {
            continue;
        }
        nfds = epoll_wait(epollfd, events, max_cap, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }
        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == listener) { //Accept connections
                connection = accept(listener, (struct sockaddr *) &addr, &len);
                //SYSLOG(CONNECTION ESTABLISHED WITH ...)
                if (connection == -1) {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                fcntl(connection, F_SETFL, fcntl(connection, F_GETFL, 0) | O_NONBLOCK);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = connection;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connection, &ev) == -1) {
                    perror("epoll_ctl: conn_sock");
                    exit(EXIT_FAILURE);
                }
            } else { process_msg(events[n].data.fd); } //Process msg from other fds (clients)
    }
        }
// void Start() {
//     int index = 0;
//     while (1) {
        
//         if (g_current_capacity == g_total_capacity ) {
//             continue;
//         }
//         index = 0;
//         while (index < g_total_capacity) {
//             if (users[index].active == 0) {

//                 int temp_handle;
//                 struct sockaddr_in temp_sock;
//                 socklen_t temp_len;
//                 // users[index].handle = accept(s_tcp->handle, (struct sockaddr *) &(users[index].addr), &(users[index].len));
//                 temp_handle = accept(s_tcp->handle, (struct sockaddr *) &temp_sock, &temp_len);
//                 // pthread_mutex_lock(&m_users);
//                 process_msg(temp_handle);

//                 // g_current_capacity = g_current_capacity+1;
//                 // char text[] ="Witam";
//                 // send(users[index].handle, text, 6, 0);
//                 // send(users[index].handle, text, 6, 0);
//                 // send(users[index].handle, text, 6, 0);
//                 // close(users[index].handle);
//             } else { index = index+1; }
//         }    
//     }
// }

// void respond() {
//     recv
// }

#endif