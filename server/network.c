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

static Server *s_udp;
static Server *s_tcp;
static User *users;
int g_total_capacity = 1;
int g_current_capacity = 0;

//Initialize all static values. Prepare the sockets and start listening on them. 
int InitServer(char *ip, int tport, int tn, int uport, int un, int capacity) {
    s_udp = (Server*) malloc(sizeof(Server));
    s_tcp = (Server*) malloc(sizeof(Server));
    g_total_capacity = capacity;
    users = (User*) malloc(sizeof(User)*g_total_capacity);
    memset(users, 0, sizeof(User)*g_total_capacity);
    memset(s_udp, 0, sizeof(Server));
    memset(s_tcp, 0, sizeof(Server));
    for (size_t i = 0; i < g_total_capacity; i++) {
        users[i].len = sizeof(users->addr);
    }

    s_udp->addr.sin_family = AF_INET;
    s_udp->addr.sin_addr.s_addr = htonl(INADDR_ANY);
    s_udp->addr.sin_port = htons(uport);
    if ( s_udp->handle = socket(AF_INET, SOCK_DGRAM, 0) == -1 ) { 
        log_info(ERROR, "UDP socket creation failure"); 
        exit(EXIT_FAILURE);
    }
    if ( bind (s_udp->handle, (struct sockaddr *) &(s_udp->addr), sizeof(s_udp->addr)) == -1 ) { 
        log_info(ERROR, "UDP socket binding failure"); 
        exit(EXIT_FAILURE);
    }
    if ( listen(s_udp->handle, un) == -1 ) { 
        log_info(ERROR, "UDP socket listening failure"); 
        exit(EXIT_FAILURE);
    }

    s_tcp->addr.sin_family = AF_INET;
    s_tcp->addr.sin_addr.s_addr = htonl(INADDR_ANY);
    s_tcp->addr.sin_port = htons(tport);
    if ( s_tcp->handle = socket(AF_INET, SOCK_STREAM, 0) == -1 ) { 
        log_info(ERROR, "TCP socket creation failure");
        exit(EXIT_FAILURE);
    }
    // if ( fcntl(s_tcp->handle, F_SETFL, fcntl(s_tcp->handle, F_GETFL, 0) | O_NONBLOCK) == -1 ) { log_info(ERROR, "TCP socket fnctl failure"); }
    if ( bind (s_tcp->handle, (struct sockaddr *) & s_tcp->addr, sizeof(s_tcp->addr)) == -1 ) { 
        log_info(ERROR, "TCP socket binding failure"); 
        exit(EXIT_FAILURE);
    }
    if ( listen(s_tcp->handle, tn) == -1 ) { 
        log_info(ERROR, "TCP socket listening failure");
        exit(EXIT_FAILURE);
    }
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
                users[index].handle = accept(s_tcp->handle, (struct sockaddr *) &(users[index].addr), &(users[index].len));
                char text[] ="Witam";
                send(users[index].handle, text, 6, 0);
                send(users[index].handle, text, 6, 0);
                send(users[index].handle, text, 6, 0);
                close(users[index].handle);
            } else { index = index+1; }
        }    
    }
}

// void respond() {
//     recv
// }

#endif