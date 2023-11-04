#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

FILE *file;
// 7070 UDP 1024 name.mp4
int main (int argc, char* argv[]) {
    int mode = 0;
    struct sockaddr_in saadr, caddr;
    int buff_size = atoi(argv[3]);
    int handle, chandle;
    char *buffer = (char*) malloc(buff_size*sizeof(char));
    int size = 10000*1024;
    int counter = 0, n = 0;
    memset(&saadr, 0, sizeof(saadr));
    memset(&caddr, 0, sizeof(caddr));
    saadr.sin_family = AF_INET;
    saadr.sin_addr.s_addr = htonl(INADDR_ANY);
    saadr.sin_port = htons(atoi(argv[1]));
    socklen_t clen = sizeof(caddr);
    socklen_t addr_size;
    if (strncmp(argv[2],"UDP", 3) == 0){
        mode = 0;
        handle = socket(AF_INET, SOCK_DGRAM, 0);
        bind (handle, (struct sockaddr *) & saadr, sizeof(saadr));
        listen(handle, 2);
        setsockopt(handle, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
        recvfrom(handle, buffer, buff_size, 0, (struct sockaddr *)&caddr, &clen);
    } else if (strncmp(argv[2],"TCP", 3) == 0) {
        mode = 1;
        handle = socket(AF_INET, SOCK_STREAM, 0);
        bind (handle, (struct sockaddr *) & saadr, sizeof(saadr));
        setsockopt(handle, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
        listen(handle, 2);
        chandle = accept(handle, (struct sockaddr *) &caddr, &addr_size);
        sleep(1);
        recv(chandle, buffer, buff_size, 0);
    } else { exit(1); }
    file = fopen(buffer, "wb");
    switch (mode) {
    case 0:
        do {        
            n = recvfrom(handle, buffer, buff_size, 0, (struct sockaddr *)&caddr, &clen);
            fwrite(buffer, sizeof(char), n, file);
            counter ++;
        } while (n == buff_size);
        break;
    case 1:
        do {        
            n = recv(chandle, buffer, buff_size, 0);
            fwrite(buffer, sizeof(char), n, file);
            counter ++;
        } while (n == buff_size);
        break;
    }
    fclose(file);
    close(handle);
    close(chandle);
    printf("Odebrano %d pakietow o wielkosci %d\n",counter, buff_size);
    return 0;
}