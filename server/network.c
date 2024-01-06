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
#include <syslog.h>
#include <poll.h>
#include "s_network.c"
#include <pthread.h>
#include <sys/epoll.h>

static Server *g_tcp; 
static User *g_users;
static Offer *g_offers;
static int *g_threadLoad;
pthread_mutex_t m_offers, m_passwd, m_users;
static pthread_t *t_threads;
static pthread_t t_clock;
static int g_offer_cap;       
static int g_users_cap;                               
int highest_id = 0;

static Passwd *passwd;
void InitPasswd(char[]);
int recv_(int, Message*, int);
int send_(int, int, Message*, int);
void *worker(void*);
void term_thread(const char*, int, int);
void process_msg(int, int);
void elect_supplier(int, Message*, int);
void receive_new(int, Message*, int);
void *clock_(void*);
int register_(int, Message*, int);
int login(int, Message*, pthread_mutex_t*, User*, int, int);
void propagate(Message*);
//DONE Initialize ports, all data structures, run threads.
int InitServer(char *ip, int tport, int tn, int threads, int user_cap, int offer_cap) {
/////////Initializing local variables///////////
    int temp;
    g_offer_cap = offer_cap;
    g_users_cap = user_cap;
    g_tcp = (Server*) malloc(sizeof(Server));
    memset(g_tcp, 0, sizeof(Server));
    g_users = (User*) malloc(sizeof(User)*user_cap);
    memset(g_users, 0, sizeof(User)*user_cap);
    g_threadLoad = (int*) malloc(sizeof(int)*threads);
    memset(g_threadLoad, 0, sizeof(int)*threads);
    g_offers = (Offer*)malloc(sizeof(Offer)*offer_cap);
    memset(g_offers, 0, sizeof(Offer)*offer_cap);
    t_threads = (pthread_t*) malloc(sizeof(pthread_t)*threads);
    pthread_mutex_init(&m_offers, NULL);
    pthread_mutex_init(&m_passwd, NULL);
    pthread_mutex_init(&m_users, NULL);
    for (size_t i = 0; i < user_cap; i++) {
        g_users[i].len = sizeof(g_users->addr);
    }
    for (size_t i = 0; i < g_offer_cap; i++) {
        g_offers[i].active_eta = 1000000;
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
    clock_conf->tLoad_ptr = g_threadLoad;
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

/* Worker function spawned as a thread. Accepts incoming connections and listens on the via epoll. */ 
void *worker(void *arg) {
    int lisneter = g_tcp->handle;
    struct epoll_event ev, events[((Tinfo*)arg)->max];
    int connection, nfds, epollfd;
    g_threadLoad[((Tinfo*)arg)->id] = 1;
    epollfd = epoll_create(((Tinfo*)arg)->max);
    if (epollfd == -1) { term_thread("EPOLL FAILED", LOG_ALERT, ((Tinfo*)arg)->id); }    
    ev.events = EPOLLIN;
    ev.data.fd = g_tcp->handle;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, g_tcp->handle, &ev) == -1) { term_thread("EPOLL FAILED", LOG_ALERT, ((Tinfo*)arg)->id); }
    while (true) { // Posibble change: based on global value; SIGKILL changes the value tho false
        if (g_threadLoad[((Tinfo*)arg)->id] == ((Tinfo*)arg)->max) { continue; }
        nfds = epoll_wait(epollfd, events, ((Tinfo*)arg)->max, -1);
        if (nfds == -1) { term_thread("EPOLL FAILED", LOG_ALERT, ((Tinfo*)arg)->id); }
        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == g_tcp->handle) { //Accept connections
                pthread_mutex_lock(&m_users);
                for (size_t i = 0; i < g_users_cap; i++) {
                    if (g_users[i].active == 0) {
                        connection = accept(g_tcp->handle, (struct sockaddr *) (&g_users[i].addr), &g_users[i].len);
                        if (connection == -1) {
                            pthread_mutex_unlock(&m_users);
                            continue;
                        } else {
                            fcntl(connection, F_SETFL, fcntl(connection, F_GETFL, 0) | O_NONBLOCK);
                            ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                            ev.data.fd = connection;
                            if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connection, &ev) == -1) {
                                pthread_mutex_unlock(&m_users);
                                continue;
                            }
                            g_users[i].handle = connection;
                            g_users[i].active = 1;
                            pthread_mutex_unlock(&m_users);
                            g_users[i].tid = ((Tinfo*)arg)->id;
                            g_users[i].timeout = 100;
                            g_users[i].logged = 0;
                            g_threadLoad[((Tinfo*)arg)->id]  = g_threadLoad[((Tinfo*)arg)->id]+1;
                        }
                    }
                }
                pthread_mutex_unlock(&m_users);
                struct sockaddr_in addr;
                socklen_t len;
                connection = accept(g_tcp->handle, (struct sockaddr *) &addr, &len);
                Message msg;
                msg.code = LIMIT_REACHED;
                send(connection, &msg, sizeof(Message), 0);
                close(connection);
            } else { 
                if (events[n].events == 8193) { send_(events[n].data.fd, USER_TIMEOUT, NULL, ((Tinfo*)arg)->id); } 
                else { process_msg(events[n].data.fd, ((Tinfo*)arg)->id); }
            }            
        }
    }
}
/* Server-side function for processing messages based on their Message::code. */
void process_msg(int connection, int tid) {
    Message msg;
    if ( recv_(connection, &msg, tid) == -1) {
        return;
    }
    switch (msg.code) {
        case REGISTER:
            register_(connection, &msg, tid);
            break;
        case LOGIN:
            login(connection, &msg, &m_users, g_users, g_users_cap, tid);
            break;
        case NEW_OFFER:
            receive_new(connection, &msg, tid);
            break;
        case ACCEPT_OFFER:
            elect_supplier(connection, &msg, tid);
            break;
        default:
            break;
    }
      
}

void receive_new(int connection, Message *msg, int tid) {
    User *temp_cli_ptr = NULL;
    for (size_t i = 0; i < g_users_cap; i++) {
        if (g_users[i].handle = connection) {
            temp_cli_ptr = &g_users[i];
        }
    }
    if (temp_cli_ptr == NULL) {
        return;
    }
    pthread_mutex_lock(&m_offers);
    for (size_t i = 0; i < g_offer_cap; i++) {
        if (g_offers[i].phase == 0) { 
            sscanf(msg->message,"%d %s %d", &g_offers[i].active_eta, g_offers[i].resource,  &g_offers[i].quantity);
            g_offers[i].phase = 1;
            g_offers[i].cli_ptr = temp_cli_ptr;
            pthread_mutex_unlock(&m_offers);
            send_(connection, NEW_ACCEPTED, NULL, tid);
            Message temp;
            sprintf(temp.message, "%d %s %d %s %d", g_offers[i].id, g_offers[i].cli_ptr->name, g_offers[i].active_eta, g_offers[i].resource, g_offers[i].quantity);
            propagate(&temp);
            return;
        }
    }
    pthread_mutex_unlock(&m_offers);
    send_(connection, NEW_DECLINED, NULL, tid);
    return;
}

/* Set the supplier with the lowest bid as the new handler for the order.
    Returns a message:
        BID_DECLINE with "{id}{lowETA}{supETA}" on failure
        BID_ACCEPT wtih "{id}" on successful election 
*/
void elect_supplier(int connection, Message *msg, int tid) {
    int id, eta;
    sscanf(msg->message, "%d %d", &id, &eta);
    for (size_t i = 0; i < g_offer_cap; i++) {
        if ( g_offers[i].phase == 0 ) { continue; }
        if ( g_offers[i].id == id ) {
            pthread_mutex_lock(&m_offers);
            if (g_offers[i].lowSup_eta <= eta) {
                pthread_mutex_unlock(&m_offers);
                Message temp_msg;
                sprintf(temp_msg.message,"%d %d %d", g_offers[i].id, g_offers[i].lowSup_eta, eta);
                send_(connection, BID_DECLINE, &temp_msg, tid);
                return;
            } else {
                g_offers[i].lowSup_eta = eta;
                for (int j = 0; j < g_users_cap; j++) {
                    if (connection == g_users[j].handle) {
                        g_offers[i].sup_ptr = &g_users[j];
                        break;
                    }
                }
                Message temp_msg;
                sprintf(temp_msg.message, "%d", id);
                send_(connection, BID_ACCEPT, &temp_msg, tid);
                return;
            }
        }
    }
    Message temp_msg;
    sprintf(temp_msg.message,"%d %d %d", 0, 0, 0);
    send_(connection, BID_DECLINE, &temp_msg, tid);
    return;
}
/*Inline function for syslog() & pthread_exit().
Terminates the current thread and logs the event. */
inline void term_thread(const char *err, int type, int id) {
    syslog(type, "T%d: %s", id, err);
    pthread_exit(NULL);
}

/*Inline function for recv(). If recovering data from the handle returns -1, 
it closes the socket and makes the user inactive. */
inline int recv_(int handle, Message *msg, int thread_id) {
    if (recv(handle, msg, sizeof(Message), 0) == -1) {
        for (size_t i = 0; i < g_users_cap; i++) {
            if (g_users[i].handle == handle && g_users[i].active == 1 ) {
                g_users[i].active = 0;
                g_threadLoad[thread_id] = g_threadLoad[thread_id]-1;
                close(handle);
                return -1;
            }
        }
    } else { return 0; }
}

/*Inline function for send(). Passing NULL as msg makes the msg code-only. Acts the same as recv_() when it encounters -1 for sending. */
inline int send_(int handle, int code, Message *msg, int thread_id){
    int clear = 0;
    if (msg == NULL) {
        clear = 1;
        msg = (Message*)malloc(sizeof(Message));
    }
    msg->code = code;
    if (send(handle, msg, sizeof(Message), 0) == -1) {
        for (size_t i = 0; i < g_users_cap; i++) {
            if (g_users[i].handle == handle) {
                pthread_mutex_lock(&m_users);
                if (g_users[i].active == 0) {
                    pthread_mutex_unlock(&m_users);
                    if (clear == 1) { free(msg); }
                    return -1;
                }
                g_users[i].active = 0;
                pthread_mutex_unlock(&m_users);
                g_threadLoad[thread_id] = g_threadLoad[thread_id]-1;
                close(handle);
                if (clear == 1) { free(msg); }
                return -1;
            }
        }
    }
    if (clear == 1) { free(msg); }
    return 0;
}

//DONE Login user by adding him to the users array. 
int login(int connection, Message *msg, pthread_mutex_t *m_users, User *s_users, int uo, int tid) {
    char login[10];
    char password[10];
    int type;
    char line_buff[BUFF_SIZE];
    char *token;
    sscanf(msg->message, "%s %s", login, password);
    pthread_mutex_lock(&(passwd->mutex));
    rewind(passwd->file);
    while (fgets(line_buff, BUFF_SIZE, passwd->file)!=NULL) {
        token = strtok(line_buff, " ");
        if (strcmp(token, login) == 0 ) {
            token = strtok(NULL, " ");
            if (strcmp(token, password) == 0) {
                token = strtok(NULL, " ");
                type = atoi(token);
                pthread_mutex_unlock(&(passwd->mutex));
                ///////////////INSERTING USER////////////////
                pthread_mutex_lock(m_users);
                for (size_t i = 0; i < uo; i++) {
                    if (s_users[i].handle == connection) {
                        s_users[i].logged = 1;
                        pthread_mutex_unlock(m_users);
                        strcpy(s_users[i].name, login);
                        s_users[i].type = type;
                        Message msg;
                        sprintf(msg.message, "%d", type);
                        send_(connection, LOGIN_SUCCESFUL, &msg, tid);
                        return 0;
                    }
                }
                ///////////// END OF INSERTING ///////////////             
            }
        }
    }
    pthread_mutex_unlock(&(passwd->mutex));
    send_(connection, LOGIN_FAILED, NULL, tid);
    close(connection);
    return 1;
}

//DONE Register user in passwd file then close the connection.
int register_(int connection, Message *msg, int tid) {
    int counter = 0;
    char *token;
    char login[10];
    char password[10];
    int type;
    Message respond;
    sscanf(msg->message, "%s %s %d", login, password, &type);
    pthread_mutex_lock(&(passwd->mutex)); //Keep the mutex for now. Connections will be handled 1by1 prolly.
    rewind(passwd->file);
    char line_buff[BUFF_SIZE];
    while (fgets(line_buff, BUFF_SIZE, passwd->file)!=NULL) { //Fail if login duplicate found
        token = strtok(line_buff, " ");
        if (strcmp(token, login) == 0 ) { // REGISTER FAILED - NAME TAKEN
                pthread_mutex_unlock(&(passwd->mutex));
                if ( send_(connection, REGISTER_FAILED, NULL, tid) == 0) {
                    g_threadLoad[tid] = g_threadLoad[tid]-1;
                }
                close(connection);
                return 1;
        }
    }
    //  fprintf(passwd->file, "%s %s %d\n", credentials.login, credentials.password, credentials.type, new_id);
    fprintf(passwd->file, "%s %s %d\n", login, password, type); //Print new credentials into passwd
    fflush(passwd->file);
    pthread_mutex_unlock(&(passwd->mutex));
    if ( send_(connection, REGISTER_SUCCESFUL, NULL, tid) == 0) {
        g_threadLoad[tid] = g_threadLoad[tid]-1;
    }
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
        //TIMEOUT COUNTDOWN
        for (size_t i = 0; i < ((Sclock*)str)->u_size; i++) { //TIMEOUT COUNTDOWN
            if ( ((Sclock*)str)->uptr[i].active == 1 && ((Sclock*)str)->uptr[i].logged == 0) {
                ((Sclock*)str)->uptr[i].timeout = ((Sclock*)str)->uptr[i].timeout - 1;
            }
            if ( ((Sclock*)str)->uptr[i].timeout < 0 ) {
                close(((Sclock*)str)->uptr[i].handle);
                ((Sclock*)str)->uptr[i].active = 0;
                ((Sclock*)str)->tLoad_ptr[((Sclock*)str)->uptr[i].tid] = ((Sclock*)str)->tLoad_ptr[((Sclock*)str)->uptr[i].tid] - 1;
            }
        }
        //OFFER OPERATIONS
        for (size_t i = 0; i < ((Sclock*)str)->o_size; i++) {
            //SKIP EMPTY OFFER
            if (((Sclock*)str)->optr[i].phase == 0 ) { continue; }
            ((Sclock*)str)->optr[i].active_eta = ((Sclock*)str)->optr[i].active_eta - 1;
            //OFFER ACTIVE TIMEOUT
            if (((Sclock*)str)->optr[i].active_eta < 0) {
                if ( ((Sclock*)str)->optr[i].lowSup_eta == 1000000) {
                    ((Sclock*)str)->optr[i].cli_ptr->busy = 0;
                    send_(((Sclock*)str)->optr[i].cli_ptr->handle, OFFER_TIMEOUT, NULL, ((Sclock*)str)->optr[i].cli_ptr->tid);
                    ((Sclock*)str)->optr[i].phase = 0;
                } else { //START TRANSACTION ON BOTH SIDES
                    Message msg;
                    sprintf(msg.message, "%d", (((Sclock*)str)->optr[i].id));
                    ((Sclock*)str)->optr[i].cli_ptr->busy = 1;
                    ((Sclock*)str)->optr[i].sup_ptr->busy = 1;
                    send_(((Sclock*)str)->optr[i].sup_ptr->handle, TRANSACTION_STARTED, &msg, ((Sclock*)str)->optr[i].sup_ptr->tid);
                    send_(((Sclock*)str)->optr[i].sup_ptr->handle, TRANSACTION_STARTED, &msg, ((Sclock*)str)->optr[i].sup_ptr->tid);
                }
            } else {
                //ID  ETA NAME RESOURCE QUA LOWETA
                printf("%d %d %s %s %d %d\n",
                ((Sclock*)str)->optr[i].id,
                ((Sclock*)str)->optr[i].active_eta,
                ((Sclock*)str)->optr[i].cli_ptr->name,
                ((Sclock*)str)->optr[i].resource,
                ((Sclock*)str)->optr[i].quantity,
                ((Sclock*)str)->optr[i].lowSup_eta
                );
            }
        }
        fflush(stdout);
    }
}

inline void propagate(Message *msg) {
    for (size_t i = 0; i < g_users_cap; i++) {
        if (g_users[i].active == 1 && g_users[i].type == SUPPLIER) {
            send_(g_users[i].handle, NEW_OFFER, msg, g_users[i].tid);
        }  
    }
}

void InitPasswd(char path[]) {
    // srand(time(NULL));
    passwd = (Passwd*) malloc(sizeof(Passwd));
    pthread_mutex_init(&(passwd->mutex), NULL);
    passwd->file = fopen(path, "a+");
}
#endif