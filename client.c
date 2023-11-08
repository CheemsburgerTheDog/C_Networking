#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
struct dzialanie {
    int a;
    int b;
};
// "
int main (int argc, char* argv[]) {
    char *test = "TCP";
    int buff_size = 100;
    int handle;
    char *buffer = (char*) malloc(buff_size*sizeof(char));
    struct sockaddr_in saddr;
    int mode = 0;
    struct dzialanie to_send;
    to_send.a = 2;
    to_send.b = 3;
    memset(&saddr, 0, sizeof (saddr));
    saddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &(saddr.sin_addr));
    saddr.sin_port = htons (7777);
    socklen_t slen= sizeof(saddr);

    if (strncmp(test, "UDP", 3) == 0){
        mode = 0;
        handle = socket (AF_INET, SOCK_DGRAM, 0);
    } else if (strncmp(test, "TCP", 3) == 0){
        mode = 1;
        handle = socket(AF_INET, SOCK_STREAM, 0);
        connect(handle, (struct sockaddr*) &saddr, sizeof(saddr));
    } else{ exit(1); }

    // while ( ( n = fread(buffer, 1, buff_size, file) ) > 0 ) {
        // if (mode == 0) { sendto(handle, buffer, n, 0, (struct sockaddr *) &saddr, slen); }
        // if (mode == 1) { send(handle, buffer, n, 0); }
    if (mode == 0) { sendto(handle, &to_send, sizeof(struct dzialanie), 0, (struct sockaddr *) &saddr, slen);}
    if (mode == 1) { send(handle, &to_send, sizeof(struct dzialanie), 0); }
    //     counter ++;
    // }
    // sendto(handle, buffer, 0, 0, (struct sockaddr *) &saddr, slen);
    // printf("Wyslano %d pakietow o wielkosci %d\n",counter, buff_size);
    close(handle);
    // fclose(file);
    return 0;
}