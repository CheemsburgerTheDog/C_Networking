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

static Server *s_tcp; 
static User *s_users;
static int *g_serverLoad;
pthread_mutex_t m_poll, m_passwd, m_users;
static pthread_t *t_threads;
static pthread_t t_clock;                                      

int recv_(int, Message*);
int send_(int, int, Message*);
void *worker(void*);
void term_thread(const char*, int, int);
void process_msg(int, int);
//DONE Initialize ports, all data structures, run threads.
int InitServer(char *ip, int tport, int tn, int threads, int total_capacity) {
/////////Initializing local variables///////////
    int temp;
    s_tcp = (Server*) malloc(sizeof(Server));
    memset(s_tcp, 0, sizeof(Server));
    s_users = (User*) malloc(sizeof(User)*total_capacity);
    memset(s_users, 0, sizeof(User)*total_capacity);
    g_serverLoad = (int*) malloc(sizeof(int)*threads);
    memset(g_serverLoad, 0, sizeof(int)*threads);
    t_threads = (pthread_t*) malloc(sizeof(pthread_t)*threads);
    pthread_mutex_init(&m_poll, NULL);
    pthread_mutex_init(&m_passwd, NULL);
    pthread_mutex_init(&m_users, NULL);
    for (size_t i = 0; i < total_capacity; i++) {
        s_users[i].len = sizeof(s_users->addr);
    }
    openlog("CHEEMS", LOG_PERROR, LOG_USER);

////////Preping server////////////////////////
    s_tcp->max_cap = total_capacity;
    s_tcp->addr.sin_family = AF_INET;
    s_tcp->addr.sin_addr.s_addr = htonl(INADDR_ANY);
    s_tcp->addr.sin_port = htons(tport);
    temp = socket(AF_INET, SOCK_STREAM, 0);
    s_tcp->handle = temp;
    fcntl(s_tcp->handle, F_SETFL, fcntl(s_tcp->handle, F_GETFL, 0) | O_NONBLOCK);
    bind (s_tcp->handle, (struct sockaddr *) & s_tcp->addr, sizeof(s_tcp->addr));
    int buffs = sizeof(Message)*((int)total_capacity*1.3);
    buffs = buffs - (buffs%(sizeof(Message)));
    setsockopt(s_tcp->handle, SOL_SOCKET, SO_RCVBUF, &buffs, sizeof(buffs));
    if ( listen(s_tcp->handle, tn) == -1) { 
        syslog(LOG_EMERG, "M %s", "Error at listening");
        exit(EXIT_FAILURE);
    }

/////////////THREAD CREATION///////////////////////////
    Sclock *clock_conf;
    clock_conf = (Sclock*)malloc(sizeof(Sclock));
    clock_conf->ptr = s_users;
    clock_conf->size = total_capacity;
    pthread_create(&t_clock, NULL, clock_, clock_conf);
    int t_modulo  = total_capacity%threads;
    int t_max_cap = (total_capacity-t_modulo)/threads;
    for (size_t i = 0; i < threads; i++) {
        Tinfo *conf;
        conf = (Tinfo*)malloc(sizeof(Tinfo));
        conf->id = i;
        if (t_modulo > 0) { 
            t_modulo = t_modulo - 1; 
            conf->max = t_max_cap+1;
        } else { conf->max = t_max_cap; }        
        pthread_create(&t_threads[i], NULL, worker, conf);     
    }
    pthread_join(t_clock, NULL);
    for (size_t i = 0; i < threads; i++) {
        pthread_join(t_threads[i], NULL);
    }
    return 0;
}

//DONE Worker thread for handling connections. 
void *worker(void *arg) {
    int lisneter = s_tcp->handle;
    struct sockaddr_in addr;
    socklen_t len;
    struct epoll_event ev, events[((Tinfo*)arg)->max];
    int connection, nfds, epollfd;
    g_serverLoad[((Tinfo*)arg)->id] = 1;
    epollfd = epoll_create(((Tinfo*)arg)->max);
    if (epollfd == -1) { term_thread("EPOLL FAILED", LOG_ALERT, ((Tinfo*)arg)->id); }    
    ev.events = EPOLLIN;
    ev.data.fd = s_tcp->handle;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, s_tcp->handle, &ev) == -1) { term_thread("EPOLL FAILED", LOG_ALERT, ((Tinfo*)arg)->id); }
    while (true) { // Posibble change: based on global value; SIGKILL changes the value tho false
        if (g_serverLoad[((Tinfo*)arg)->id] == ((Tinfo*)arg)->max) { continue; }
        nfds = epoll_wait(epollfd, events, ((Tinfo*)arg)->max, -1);
        if (nfds == -1) { term_thread("EPOLL FAILED", LOG_ALERT, ((Tinfo*)arg)->id); }
        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == s_tcp->handle) { //Accept connections
                connection = accept(s_tcp->handle, (struct sockaddr *) &addr, &len);
                if (connection == -1) { 
                    continue;
                } else { g_serverLoad[((Tinfo*)arg)->id]  = g_serverLoad[((Tinfo*)arg)->id]+1; }
                fcntl(connection, F_SETFL, fcntl(connection, F_GETFL, 0) | O_NONBLOCK);
                ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                ev.data.fd = connection;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connection, &ev) == -1) {
                    perror("epoll_ctl: conn_sock");
                    exit(EXIT_FAILURE);
                }
            } else { 
                if (events[n].events == 8193) { close(events[n].data.fd); }
                else { process_msg(events[n].data.fd, ((Tinfo*)arg)->id); } //Process msg from other fds (clients) }
            }            
    }
        }
}

//VIP ADD OTHER CODES
void process_msg(int connection, int thread_id) {
    Message msg;
    if ( recv_(connection, &msg) == -1) {
        return;
    }
    switch (msg.code) {
        case REGISTER:
            register_(connection, &msg);
            break;
        case LOGIN:
            login(connection, &msg, thread_id, &m_users, s_users);
            break;
        case NEW_OFFER:
            add_offer(connection, &msg, );
            break;
        case ACCEPT_OFFER:
            elect_supplier(connection, &msg, &m_users, s_users, &m_offers, s_offer);
        default:
            break;
    }
      
}
//DONE Inline function for syslog() & pthread_exit()
inline void term_thread(const char *err, int type, int id) {
    syslog(type, "T%d: %s", id, err);
    pthread_exit(NULL);
}

//DONE Inline function for recv(). If recovering data from the handle returns -1 it closes the socket and makes the user inactive;
inline int recv_(int handle, Message *msg){
    if (recv(handle, msg, sizeof(Message), 0) == -1) {
        int i = 0;
        while (1) {
            if (s_users[i].handle == handle) {
                s_users[i].active = false;
                close(handle);
                return -1;
            } 
       }
    } else { return 0; }
}

//DONE Inline function for send(). Passing NULL as msg makes the msg code-only. Acts the same as recv_() when it encounters -1 for sending;
inline int send_(int handle, int code, Message *msg){
    int clear = 0;
    if (msg == NULL) {
        clear = 1;
        msg = (Message*)malloc(sizeof(Message));
    }
    msg->code = code;
    if (send(handle, msg, sizeof(Message), 0) == -1) {
        int i = 0;
        while (1) {
            if (s_users[i].handle == handle) {
                close(handle);
                if (clear == 1) { free(msg); }
                return -1;
            } 
        }
    } else {
        if (clear == 1) { free(msg); } 
        return 0; 
    }
}
#endif