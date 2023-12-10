#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include "/home/cheemsburger/Desktop/C_Networking/server/s_network.c"

// "127.0.0.1 7777 UDP"
int main (int argc, char* argv[]) {
    char received[6];
    int handle, mode = 0;
    struct sockaddr_in saddr;

    memset(&saddr, 0, sizeof (saddr));
    saddr.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &(saddr.sin_addr));
    saddr.sin_port = htons (atoi(argv[2]));
    socklen_t slen= sizeof(saddr);

    if (strncmp(argv[3], "UDP", 3) == 0){
        mode = 0;
        handle = socket (AF_INET, SOCK_DGRAM, 0);
    } else if (strncmp(argv[3], "TCP", 3) == 0){
        mode = 1;
        handle = socket(AF_INET, SOCK_STREAM, 0);
        connect(handle, (struct sockaddr*) &saddr, sizeof(saddr));
    } else{ exit(1); }

    // while ( ( n = fread(buffer, 1, buff_size, file) ) > 0 ) {
        // if (mode == 0) { sendto(handle, buffer, n, 0, (struct sockaddr *) &saddr, slen); }
        // if (mode == 1) { send(handle, buffer, n, 0); }
    if (mode == 0) { recvfrom(handle, &received, 6, 0, (struct sockaddr *) &saddr, slen);}
    if (mode == 1) { 
        Message msg;
        msg.type = REGISTER;
        char login[10];
        char regi[10];
        scanf("%s", login);
        scanf("%s", regi);
        sprintf(msg.message, "%s %s %d", login, regi, 1);
        send(handle, &msg, sizeof(Message), 0);
        fflush(stdout); 
    }
    //     counter ++;
    // }
    // sendto(handle, buffer, 0, 0, (struct sockaddr *) &saddr, slen);
    // printf("Wyslano %d pakietow o wielkosci %d\n",counter, buff_size);
    close(handle);
    // fclose(file);
    return 0;
}