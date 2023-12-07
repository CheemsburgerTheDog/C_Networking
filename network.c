#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <poll.h>
#include <logging.c>
#include <s_network.c>

static Server *udp;
static Server *tcp;
static User *users;


int InitServer(char *ip, int tport, int tn, int uport, int un, int capacity) {
    udp = (Server*) malloc(sizeof(Server));
    tcp = (Server*) malloc(sizeof(Server));
    users = (User*) malloc(sizeof(User)*capacity);

    udp->addr.sin_family = AF_INET;
    udp->addr.sin_addr.s_addr = htonl(INADDR_ANY);
    udp->addr.sin_port = htons(uport);
    udp->len = sizeof(udp->addr);
    if ( udp->handle = socket(AF_INET, SOCK_DGRAM, 0) == -1 ) { log(ERROR, "UDP socket creation failure"); }
    if ( bind (udp->handle, (struct sockaddr *) &udp->addr, udp->len) == -1 ) { log(ERROR, "UDP socket binding failure"); }
    if ( listen(udp->handle, un) == -1 ) { log(ERROR, "UDP socket listening failure"); }

    tcp->addr.sin_family = AF_INET;
    tcp->addr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcp->addr.sin_port = htons(tport);
    tcp->len = sizeof(tcp->addr);
    if ( tcp->handle = socket(AF_INET, SOCK_STREAM, 0) == -1 ) { log(ERROR, "TCP socket creation failure"); }
    if ( fcntl(tcp->handle, F_SETFL, fcntl(tcp->handle, F_GETFL, 0) | O_NONBLOCK) == -1 ) { log(ERROR, "TCP socket fnctl failure"); }
    if ( bind (tcp->handle, (struct sockaddr *) & tcp->addr, tcp->len) == -1 ) { log(ERROR, "TCP socket binding failure"); }
    if ( listen(tcp->handle, tn) == -1 ) { log(ERROR, "TCP socket listening failure"); }
}

void test() {
    
    while (1) {
        if ( user.handle = accept(tcp->handle, (struct sockaddr *) &(user.addr))) != -1) {
            /* code */
        }
        
    }
    
}
// void respond() {
//     recv
// }

