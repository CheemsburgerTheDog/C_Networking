#include <arpa/inet.h> 
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> 
#include <sys/socket.h>
#include <unistd.h> 
#include <stdbool.h>
#include <poll.h>
#include <string.h>

#define SIZE_OFFERARRAY 100
// #define SIZE_USERARRAY 100
#define SIZE_MSGCODE 20
#define SIZE_MSGMSG 40

typedef struct Offer;
typedef struct Message;
typedef struct Myself;
typedef struct TCP_Socket;
typedef struct UDP_Socket;
typedef struct Timer;

Offer offer_list[SIZE_OFFERARRAY];

void udp_init(UDP_Socket *);
void timer() {
    while (1) {
    system("clear");
    while (timer_paused) {}; //Block till stdin is being used. For STDOUT clarity;
    if (user_mode == 1) { printf("1. Accept offer\n2. Exit \n"); }
    else { printf("1. Publish offer\n2. Exit \n"); }
    for (int i = 0; i<LISTSIZE; i++) {
        if (offer_list[i].is_active == true) {
            printf("%d %s %d\n", offer_list[i].offer_id, offer_list[i].item, offer_list[i].time);
            offer_list[i].time -=1;
            if (offer_list[i].time <= 0) { offer_list[i].is_active = false; }
        } 
        }
        fflush(stdout);
        sleep(2);
    }


}
int main() {
    

}

typedef struct {
    char code[SIZE_MSGCODE];
    char message[SIZE_MSGMSG];
} Offer;

typedef struct {
    int handle;
    struct sockaddr_in saddr;
    socklen_t slen;
} UDP_Socket;

void udp_init(UDP_Socket *sock) {
    memset(&(sock->saddr), 0, sizeof(sock->saddr) );
    sock->saddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &(sock->saddr.sin_addr));
    sock->saddr.sin_port = htons(7777);
    sock->slen = sizeof(sock->saddr);
    sock->handle = socket(AF_INET, SOCK_DGRAM, 0);
}

