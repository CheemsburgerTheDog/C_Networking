#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
// "127.0.0.1 7777 1024 to_send.mp4 receivec.mp4"
int main (int argc, char* argv[]) {
    int buff_size = atoi(argv[3]), handle, n = 0, counter = 0;
    char *buffer = (char*) malloc(buff_size*sizeof(char));
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof (saddr));
    saddr.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &(saddr.sin_addr));
    saddr.sin_port = htons (atoi(argv[2]));
    socklen_t slen= sizeof(saddr);
    handle = socket (AF_INET, SOCK_DGRAM, 0);
    sendto(handle, argv[5], sizeof(argv[6])+10, 0, (struct sockaddr *) &saddr, slen);
    FILE *file;
    file = fopen(argv[4],"rb");
    while ( ( n = fread(buffer, 1, buff_size, file) ) > 0 ) {
        sendto(handle, buffer, n, 0, (struct sockaddr *) &saddr, slen);
        counter ++;
    }
    printf("Wyslano %d pakietow o wielkosci %d\n",counter, buff_size);
    fclose(file);
    return 0;
}