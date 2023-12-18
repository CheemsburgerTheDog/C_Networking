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
static User *s_users;
static int *s_capacities;
pthread_mutex_t m_poll, m_passwd, m_users;

int recv_(int, Message*);
void worker(int, int, int);
void term_thread(const char*, int, int);

void process_msg(int connection, int thread_id) {
    Message msg;
    if ( recv_(connection, &msg) == -1) {
        return;
    };
    
    switch (msg.code) {
        case REGISTER: //REGISTER
            register_(connection, &msg);
            break;
        case LOGIN: //LOGIN
            login(connection, &msg, thread_id, &m_users, s_users);
            break;
        default:
            break;
    }
      
}
//Initialize ports, all data structures, run threads
int InitServer(char *ip, int tport, int tn, int uport, int un, int threads, int total_capacity) {
    openlog("CHEEMS", LOG_PERROR, LOG_USER);
    int temp;
    s_udp = (Server*) malloc(sizeof(Server));
    s_tcp = (Server*) malloc(sizeof(Server));
    s_users = (User*) malloc(sizeof(User)*total_capacity);
    s_capacities = (int*) malloc(sizeof(int)*threads);
    memset(s_capacities, 0, sizeof(int)*threads);
    memset(s_users, 0, sizeof(User)*total_capacity);
    memset(s_udp, 0, sizeof(Server));
    memset(s_tcp, 0, sizeof(Server));

    pthread_mutex_init(&m_poll, NULL);
    pthread_mutex_init(&m_passwd, NULL);
    pthread_mutex_init(&m_users, NULL);

    for (size_t i = 0; i < total_capacity; i++) {
        s_users[i].len = sizeof(s_users->addr);
    }

    s_tcp->addr.sin_family = AF_INET;
    s_tcp->addr.sin_addr.s_addr = htonl(INADDR_ANY);
    s_tcp->addr.sin_port = htons(tport);
    temp = socket(AF_INET, SOCK_STREAM, 0);
    s_tcp->handle = temp;
    fcntl(s_tcp->handle, F_SETFL, fcntl(s_tcp->handle, F_GETFL, 0) | O_NONBLOCK);
    bind (s_tcp->handle, (struct sockaddr *) & s_tcp->addr, sizeof(s_tcp->addr));
    if ( listen(s_tcp->handle, tn) == -1){ syslog(LOG_EMERG, "M %s", "Error at listening"); }
    int modulo  = total_capacity%threads;
    int max_cap = (total_capacity-modulo)/threads;
    worker(s_tcp->handle, 10, 0);

    while (modulo>0) {
        modulo = modulo -1;
    }
    

}
//Worker thread for handling connections. 
void worker(int listener, int max_cap, int thread_id ) {
    struct sockaddr_in addr;
    socklen_t len;
    struct epoll_event ev, events[max_cap];
    int connection, nfds, epollfd;
    s_capacities[thread_id] = 1;
    epollfd = epoll_create(max_cap);
    if (epollfd == -1) { term_thread("EPOLL FAILED", LOG_ALERT, thread_id); }    
    ev.events = EPOLLIN;
    ev.data.fd = listener;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listener, &ev) == -1) { term_thread("EPOLL FAILED", LOG_ALERT, thread_id); }
    while (true) { // Posibble change: based on global value; SIGKILL changes the value tho false
        if (s_capacities[thread_id] == max_cap) { continue; }
        nfds = epoll_wait(epollfd, events, max_cap, -1);
        if (nfds == -1) { term_thread("EPOLL FAILED", LOG_ALERT, thread_id); }
        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == listener) { //Accept connections
                connection = accept(listener, (struct sockaddr *) &addr, &len);
                if (connection == -1) { 
                    continue;
                } else { s_capacities[thread_id]  = s_capacities[thread_id]+1; }
                fcntl(connection, F_SETFL, fcntl(connection, F_GETFL, 0) | O_NONBLOCK);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = connection;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connection, &ev) == -1) {
                    perror("epoll_ctl: conn_sock");
                    exit(EXIT_FAILURE);
                }
            } else { process_msg(events[n].data.fd, thread_id); } //Process msg from other fds (clients)
    }
        }
}
inline void term_thread(const char *err, int type, int id) {
    syslog(type, "T%d: %s", id, err);
    pthread_exit(NULL);
}
int recv_(int handle, Message *msg){
    if (recv(handle, msg, sizeof(Message), 0) == -1) {
        int i = 0;
        while (1) {
            if (s_users[i].handle == handle) {
                close(handle);
                return -1;
            } 
       }
    } else { return 0; }
}
#endif