#include "structs.c"
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
#include <pthread.h>
#include <sys/epoll.h>

#define CLIENTS_SIZE 5
#define PORT 7030
#define THREADS 1
#define USER_CAP 10
#define OFFER_CAP 10

static Server *g_tcp; 
static User *g_users;
static Offer *g_offers;
static int *g_threadLoad;
pthread_mutex_t m_offers, m_users, m_passwd;
pthread_t *t_threads;
pthread_t t_clock;
static int g_offer_cap;       
static int g_users_cap;                               
int highest_id = 1;
FILE *passwd;

int InitServer(int, int, int, int, int);
    void *clock_(void*);
    void *worker(void*);
        void process_msg(int, Message*, int);
            // Sorting
            void receive_new(int , Message *, int );
            //Authentication
            void login(int, Message*, int);
            void register_(int, Message*, int);
            // Offer manipulation
            void elect_supplier(int , Message*, int );
            void finalize(int, Message*, int);



    int recv_(int, Message*, int);
    int send_(int, int, Message*, int);
    void term_thread(const char *, int , int );

int main() {
    InitServer(PORT, CLIENTS_SIZE, THREADS, USER_CAP, OFFER_CAP);
}

int InitServer(int tport, int tn, int threads, int user_cap, int offer_cap) {
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
        g_offers[i].lowest = -1;
    }
    
    openlog("CHEEMS", LOG_PERROR, LOG_USER);
    passwd = fopen("/home/ps4/Pobrane/C_Networking-Projekt_V3", "a+");
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
    setsockopt(g_tcp->handle, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if ( listen(g_tcp->handle, tn) == -1) { 
        syslog(LOG_EMERG, "M %s", "Error at listening");
        exit(EXIT_FAILURE);
    }
/////////////THREAD CREATION///////////////////////////
    pthread_create(&t_clock, NULL, clock_, NULL);
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
    for (size_t i = 0; i < threads; i++) {
        pthread_join(t_threads[i], NULL);
    }
    pthread_join(t_clock, NULL);
    return 0;
}

void *worker(void *arg) {
    struct epoll_event ev, events[((Tinfo*)arg)->max];
    int connection, nfds, epollfd;
    g_threadLoad[((Tinfo*)arg)->id] = 1;
    epollfd = epoll_create(((Tinfo*)arg)->max);
    if (epollfd == -1) { term_thread("EPOLL FAILED", LOG_ALERT, ((Tinfo*)arg)->id); }    
    ev.events = EPOLLIN;
    ev.data.fd = g_tcp->handle;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, g_tcp->handle, &ev) == -1) { term_thread("EPOLL FAILED", LOG_ALERT, ((Tinfo*)arg)->id); }
    
    while (true) { // Posibble change: based on global value; SIGKILL changes the value tho false
        nfds = epoll_wait(epollfd, events, ((Tinfo*)arg)->max, -1);
        if (nfds == -1) { term_thread("EPOLL FAILED", LOG_ALERT, ((Tinfo*)arg)->id); }
        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == g_tcp->handle) { //Accept connections
                pthread_mutex_lock(&m_users);
                for (size_t i = 0; i < g_users_cap+1; i++) {
                    if (i == g_users_cap) {
                        pthread_mutex_unlock(&m_users);
                        struct sockaddr_in addr;
                        socklen_t len;
                        connection = accept(g_tcp->handle, (struct sockaddr *) &addr, &len);
                        Message msg;
                        msg.code = LIMIT_REACHED;
                        send(connection, &msg, sizeof(Message), 0);
                        close(connection);
                        continue;
                    }
                    if (g_users[i].active == 0) {
                        connection = accept(g_tcp->handle, (struct sockaddr *) (&g_users[i].addr), &g_users[i].len);
                        if (connection == -1) {
                            pthread_mutex_unlock(&m_users);
                            continue;
                        }
                        fcntl(connection, F_SETFL, fcntl(connection, F_GETFL, 0) | O_NONBLOCK);
                        ev.events = EPOLLIN | EPOLLRDHUP;
                        ev.data.fd = connection;
                        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connection, &ev) == -1) {
                            pthread_mutex_unlock(&m_users);
                            continue;
                        }
                        g_users[i].handle = connection;
                        g_users[i].active = 1;
                        g_users[i].log_timeout = 300;
                        pthread_mutex_unlock(&m_users);
                        g_users[i].tid = ((Tinfo*)arg)->id;
                        g_threadLoad[((Tinfo*)arg)->id]  = g_threadLoad[((Tinfo*)arg)->id]+1;
                        break;
                    }
                } 
            } else { 
                Message received;
                if (events[n].events == 1) {
                    if (recv_(events[n].data.fd, &received, ((Tinfo*)arg)->id) == -1) { continue; }
                        process_msg(events[n].data.fd, &received, ((Tinfo*)arg)->id); 
                } else {
                    close(events[n].data.fd);
                    g_threadLoad[((Tinfo*)arg)->id]-=1;
                    continue;
                }
            }
        }
    }            
}
 
void process_msg(int connection, Message* msg, int tid) {
    switch (msg->code) {
        case REGISTER:
            register_(connection, msg, tid);
            break;
        case LOGIN:
            login(connection, msg, tid);
            break;
        case NEW_OFFER:
            receive_new(connection, msg, tid);
            break;
        case ACCEPT_OFFER:
            elect_supplier(connection, msg, tid);
            break;
        case TRANSACTION_FINISHED:
            finalize(connection, msg, tid);
            break;
        default:
            break;
    }
      
}


void login(int connection, Message *msg, int tid) {
    char login[10];
    char password[10];
    int type;
    char line_buff[25];
    char *token;
    sscanf(msg->message, "%s %s", login, password);
    pthread_mutex_lock(&m_passwd);
    rewind(passwd);
    while (fgets(line_buff, 25, passwd)!=NULL) {
        token = strtok(line_buff, " ");
        if (strcmp(token, login) == 0 ) {
            token = strtok(NULL, " ");
            if (strcmp(token, password) == 0) {
                token = strtok(NULL, " ");
                type = atoi(token);
                pthread_mutex_unlock(&m_passwd);
                ///////////////INSERTING USER////////////////
                pthread_mutex_lock(&m_users);
                for (size_t i = 0; i < g_users_cap; i++) {
                    if (g_users[i].handle == connection) {
                        g_users[i].active = 2;
                        pthread_mutex_unlock(&m_users);
                        strcpy(g_users[i].name, login);
                        g_users[i].type = type;
                        Message msg;
                        sprintf(msg.message, "%d", type);
                        send_(connection, LOGIN_SUCCESFUL, &msg, tid);
                        return;
                    }
                }
                pthread_mutex_unlock(&m_users);
                ///////////// END OF INSERTING ///////////////             
            }
        }
    }
    pthread_mutex_unlock(&m_passwd);
    Message temp;
    send_(connection, LOGIN_FAILED, &temp, tid);
    close(connection);
    return;
}
void register_(int connection, Message *msg, int tid) {
    char *token;
    char login[10];
    char password[10];
    int type;
    Message temp;
    sscanf(msg->message, "%s %s %d", login, password, &type);
    pthread_mutex_lock(&m_passwd); //Keep the mutex for now. Connections will be handled 1by1 prolly.
    rewind(passwd);
    char line_buff[25];
    while (fgets(line_buff, 25, passwd)!=NULL) { //Fail if login duplicate found
        token = strtok(line_buff, " ");
        if (strcmp(token, login) == 0 ) { // REGISTER FAILED - NAME TAKEN
            pthread_mutex_unlock(&m_passwd);
            if ( send_(connection, REGISTER_FAILED, &temp, tid) == 0) {
                g_threadLoad[tid] = g_threadLoad[tid]-1;
                close(connection);
                return ;
            }
        }
    }
    fprintf(passwd, "%s %s %d\n", login, password, type); //Print new credentials into passwd
    fflush(passwd);
    pthread_mutex_unlock(&m_passwd);
    if ( send_(connection, REGISTER_SUCCESFUL, &temp, tid) == 0) {
        g_threadLoad[tid] = g_threadLoad[tid]-1;
        close(connection);
        return;
    }
    return;
}







void receive_new(int connection, Message *msg, int tid) {
    User *temp_cli_ptr = NULL;
    for (size_t i = 0; i < g_users_cap; i++) {
        if (g_users[i].handle == connection) {
            temp_cli_ptr = &g_users[i];
            break;
        }
    }
    if (temp_cli_ptr == NULL) {
        return;
    }
    pthread_mutex_lock(&m_offers);
    for (size_t i = 0; i < g_offer_cap; i++) {
        if (g_offers[i].phase == 0) { 
            g_offers[i].phase = 1;
            g_offers[i].id = highest_id;
            highest_id = highest_id+1;
            pthread_mutex_unlock(&m_offers);
            sscanf(msg->message,"%d %s %d", &g_offers[i].start_in, g_offers[i].resource,  &g_offers[i].quantity);
            g_offers[i].cli_ptr = temp_cli_ptr;
            Message temp;
            if (send_(connection, NEW_ACCEPTED, &temp, tid) == -1) {
                g_offers[i].phase = 0;
                return;
            }
            printf("%d %s\n", g_offers[i].id, g_offers[i].resource);
            fflush(stdout);
            g_offers[i].phase = 2;
            temp.code = NEW_OFFER;
            sprintf(temp.message, "%d %s %d %s %d", g_offers[i].id, g_offers[i].cli_ptr->name, g_offers[i].start_in, g_offers[i].resource, g_offers[i].quantity);
            for (size_t j = 0; j < g_users_cap; j++) {
                if (g_users[j].active == 2 && g_users[j].type == SUPPLIER) {
                    send_(g_users[j].handle, NEW_OFFER, &temp, tid);
                }
            }
            return;
        }           
    }
    pthread_mutex_unlock(&m_offers);
    Message temp3;
    send_(connection, NEW_DECLINED, &temp3, tid);
    return;
}

void elect_supplier(int connection, Message *msg, int tid) {
    int id, eta;
    Message temp_msg;
    sscanf(msg->message, "%d %d", &id, &eta);
    for (size_t i = 0; i < g_offer_cap; i++) {
        if ( g_offers[i].phase < 2 ) { continue; }
        if ( g_offers[i].id == id ) {
            pthread_mutex_lock(&m_offers);
            Message temp_msg;
            sprintf(temp_msg.message,"%d %d %d", g_offers[i].id, g_offers[i].lowest, eta);
            if( (g_offers[i].lowest > eta) && (g_offers[i].phase == 3) ) {
                send_(g_offers[i].sup_ptr->handle, BID_BYE, &temp_msg, g_offers[i].sup_ptr->tid); 
            }
            if( (g_offers[i].lowest > eta) || (g_offers[i].phase == 2) ) {
                g_offers[i].phase = 3;
                g_offers[i].lowest = eta;
                for (int j = 0; j < g_users_cap; j++) {
                    if (connection == g_users[j].handle) {
                        g_offers[i].sup_ptr = &g_users[j];
                        break;
                    }
                }
                pthread_mutex_unlock(&m_offers);
                send_(connection, BID_ACCEPT, &temp_msg, tid);
                return;
            }
            if (g_offers[i].lowest <= eta) {
                pthread_mutex_unlock(&m_offers);
                send_(connection, BID_DECLINE, &temp_msg, tid);
                return;
            }
        }
    }
    pthread_mutex_unlock(&m_offers);
    sprintf(temp_msg.message,"%d %d %d", 0, 0, 0);
    send_(connection, BID_DECLINE, &temp_msg, tid);
    return;
}

void finalize(int connection, Message *msg, int tid) {
    int id;
    sscanf(msg->message,"%d", &id);
    for (size_t i = 0; i < g_offer_cap; i++) {
        if (g_offers[i].phase != 4) { continue; }
        if ( g_offers[i].id == id ) {
            send_(g_offers[i].cli_ptr->handle, TRANSACTION_FINISHED, msg, g_offers[i].cli_ptr->tid);
            g_offers[i].phase = 0; 
        } 
    }
}

void *clock_(void*){
    Message temp;
    while (1) {
        sleep(1);
        system("clear");
        printf("ID  C_NAME   RES   AMT  S_NAME   LOW  ST_IN\n");
        for (size_t i = 0; i < g_offer_cap; i++) {
            if (g_offers[i].phase < 2) { continue; }
            g_offers[i].start_in = g_offers[i].start_in-1;
            if ( g_offers[i].phase == 2 ) { //PRINT AVAILABLE OFFERS
                printf("%d  %s  %s  %d  NONE  %d  %d\n",
                g_offers[i].id,
                g_offers[i].cli_ptr->name,
                g_offers[i].resource,
                g_offers[i].quantity,
                g_offers[i].lowest, 
                g_offers[i].start_in);
            }
            if (g_offers[i].phase == 3) {
                printf("%d  %s  %s  %d  %s  %d  %d\n",
                g_offers[i].id,
                g_offers[i].cli_ptr->name,
                g_offers[i].resource,
                g_offers[i].quantity,
                g_offers[i].sup_ptr->name,
                g_offers[i].lowest, 
                g_offers[i].start_in);
            }
            
            if (g_offers[i].start_in <= 0) {
                if (g_offers[i].phase == 2) { //UNTOUCHED OFFER TIMEOUT
                    send_(g_offers[i].cli_ptr->handle, OFFER_TIMEOUT, &temp, g_offers[i].cli_ptr->tid); 
                    g_offers[i].phase = 0;
                }
                if (g_offers[i].phase == 3) { // TOUCHED OFFER TRANSACT STATY
                    sprintf(temp.message, "%d %d %s", g_offers[i].id, g_offers[i].lowest, g_offers[i].sup_ptr->name);
                    send_(g_offers[i].cli_ptr->handle, TRANSACTION_STARTED, &temp, g_offers[i].cli_ptr->tid); //TO CLI
                    sprintf(temp.message, "%d %d %s", g_offers[i].id, g_offers[i].lowest, g_offers[i].cli_ptr->name);
                    send_(g_offers[i].sup_ptr->handle, TRANSACTION_STARTED, &temp, g_offers[i].sup_ptr->tid); //TO SUP
                    g_offers[i].phase = 4;
                    g_offers[i].comp_in = g_offers[i].lowest+10;
                }
                if (g_offers[i].phase == 4) {
                    g_offers[i].comp_in = g_offers[i].comp_in-1;
                    if (g_offers[i].comp_in <= 0) { //NO RESPONED FROM THE SUPPLIER MID-TRANSACTION
                        send_(g_offers[i].cli_ptr->handle, TRASACTION_ABANDON, &temp, g_offers[i].cli_ptr->tid);
                        send_(g_offers[i].sup_ptr->handle, TRASACTION_ABANDON, &temp, g_offers[i].sup_ptr->tid);
                        g_offers[i].phase = 0;
                    }
                }
            }
        }   
    }
}


















int recv_(int handle, Message *msg, int thread_id) {
    if (recv(handle, msg, sizeof(Message), MSG_NOSIGNAL) == -1) {
        for (size_t i = 0; i < g_users_cap; i++) {
            if (g_users[i].handle == handle && g_users[i].active == 1 ) {
                g_users[i].active = 0;
                g_threadLoad[thread_id] = g_threadLoad[thread_id]-1;
                close(handle);
                return -1;
            }
        }
    }
    return 0; 
}
int send_(int handle, int code, Message *msg, int thread_id){
    msg->code = code;
    if (send(handle, msg, sizeof(Message), MSG_NOSIGNAL) == -1) {
        for (size_t i = 0; i < g_users_cap; i++) {
            if (g_users[i].handle == handle && g_users[i].active == 1 ) {
                g_users[i].active = 0;
                g_threadLoad[thread_id] = g_threadLoad[thread_id]-1;
                close(handle);
                return -1;
            }
        }
    }
    return 0;
}
void term_thread(const char *err, int type, int id) {
    syslog(type, "T%d: %s", id, err);
    pthread_exit(NULL);
}