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
#include "offer.c"
#include "authentication.c"
#include <pthread.h>
#include <sys/epoll.h>

static Server *g_tcp; 
static User *g_users;
static Offer *g_offers;
static int *g_serverLoad;
pthread_mutex_t m_offers, m_passwd, m_users;
static pthread_t *t_threads;
static pthread_t t_clock;
static int g_offer_cap;       
static int g_user_cap;                               

int recv_(int, Message*);
int send_(int, int, Message*);
void *worker(void*);
void term_thread(const char*, int, int);
void process_msg(int, int);
//DONE Initialize ports, all data structures, run threads.
int InitServer(char *ip, int tport, int tn, int threads, int user_cap, int offer_cap) {
/////////Initializing local variables///////////
    int temp;
    g_offer_cap = offer_cap;
    g_user_cap = user_cap;
    g_tcp = (Server*) malloc(sizeof(Server));
    memset(g_tcp, 0, sizeof(Server));
    g_users = (User*) malloc(sizeof(User)*user_cap);
    memset(g_users, 0, sizeof(User)*user_cap);
    g_serverLoad = (int*) malloc(sizeof(int)*threads);
    memset(g_serverLoad, 0, sizeof(int)*threads);
    g_offers = (Offer*)malloc(sizeof(Offer)*offer_cap);
    memset(g_offers, 0, sizeof(Offer)*offer_cap);
    t_threads = (pthread_t*) malloc(sizeof(pthread_t)*threads);
    pthread_mutex_init(&m_offers, NULL);
    pthread_mutex_init(&m_passwd, NULL);
    pthread_mutex_init(&m_users, NULL);
    for (size_t i = 0; i < user_cap; i++) {
        g_users[i].len = sizeof(g_users->addr);
    }
    for (size_t i = 0; i < offer_cap; i++) {
        g_offers[i].phase = 2;
    }
    openlog("CHEEMS", LOG_PERROR, LOG_USER);

////////Preping server////////////////////////
    g_tcp->max_cap = user_cap;
    g_tcp->addr.sin_family = AF_INET;
    g_tcp->addr.sin_addr.s_addr = htonl(INADDR_ANY);
    g_tcp->addr.sin_port = htons(tport);
    temp = socket(AF_INET, SOCK_STREAM, 0);
    g_tcp->handle = temp;
    fcntl(g_tcp->handle, F_SETFL, fcntl(g_tcp->handle, F_GETFL, 0) | O_NONBLOCK);
    bind (g_tcp->handle, (struct sockaddr *) & g_tcp->addr, sizeof(g_tcp->addr));
    int buffs = sizeof(Message)*((int)user_cap*1.3);
    buffs = buffs - (buffs%(sizeof(Message)));
    setsockopt(g_tcp->handle, SOL_SOCKET, SO_RCVBUF, &buffs, sizeof(buffs));
    if ( listen(g_tcp->handle, tn) == -1) { 
        syslog(LOG_EMERG, "M %s", "Error at listening");
        exit(EXIT_FAILURE);
    }

/////////////THREAD CREATION///////////////////////////
    Sclock *clock_conf;
    clock_conf = (Sclock*)malloc(sizeof(Sclock));
    clock_conf->uptr = g_users;
    clock_conf->u_size = user_cap;
    clock_conf->optr = g_offers;
    clock_conf->o_size = offer_cap;
    pthread_create(&t_clock, NULL, clock_, clock_conf);
    int t_modulo  = user_cap%threads;
    int t_max_cap = (user_cap-t_modulo)/threads;
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
    int lisneter = g_tcp->handle;
    struct sockaddr_in addr;
    socklen_t len;
    struct epoll_event ev, events[((Tinfo*)arg)->max];
    int connection, nfds, epollfd;
    g_serverLoad[((Tinfo*)arg)->id] = 1;
    epollfd = epoll_create(((Tinfo*)arg)->max);
    if (epollfd == -1) { term_thread("EPOLL FAILED", LOG_ALERT, ((Tinfo*)arg)->id); }    
    ev.events = EPOLLIN;
    ev.data.fd = g_tcp->handle;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, g_tcp->handle, &ev) == -1) { term_thread("EPOLL FAILED", LOG_ALERT, ((Tinfo*)arg)->id); }
    while (true) { // Posibble change: based on global value; SIGKILL changes the value tho false
        if (g_serverLoad[((Tinfo*)arg)->id] == ((Tinfo*)arg)->max) { continue; }
        nfds = epoll_wait(epollfd, events, ((Tinfo*)arg)->max, -1);
        if (nfds == -1) { term_thread("EPOLL FAILED", LOG_ALERT, ((Tinfo*)arg)->id); }
        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == g_tcp->handle) { //Accept connections
                connection = accept(g_tcp->handle, (struct sockaddr *) &addr, &len);
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
            login(connection, &msg, thread_id, &m_users, g_users);
            break;
        case NEW_OFFER:
            receive_new(connection, &msg, g_offers, g_offer_cap, g_users, g_user_cap, &m_offers);
            break;
        case ACCEPT_OFFER:
            elect_supplier(connection, &msg);
            break;
        default:
            break;
    }
      
}
void elect_supplier(int connection, Message *msg, Offer *o_list, int on) {
    int id, eta;
    sscanf(msg->message, "%d %d", &id, &eta);
    for (size_t i = 0; i < on; i++) {
        if ( o_list[i].phase == 2 ) { continue; }
        if ( o_list[i].id == id && o_list[i].lowSup_eta <= eta ) {
            Message temp_msg;
            sprintf(temp_msg.message,"%d %d %D", &o_list[i].id, &o_list[i].lowSup_eta, &id);
            send_(connection, BID_ETA, &temp_msg);
            return;
        } else {
            //OFFERLIST ZEROED SET IT MAX VALUE ETC. 100000
            o_list[i].lowSup_eta = eta;
            o_list[i].sup_handle = connection;
        }
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
            if (g_users[i].handle == handle) {
                g_users[i].active = false;
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
            if (g_users[i].handle == handle) {
                g_users[i].active = false;
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