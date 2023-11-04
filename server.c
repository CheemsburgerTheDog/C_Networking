#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

FILE *file;
// 7070 1024
int main (int argc, char* argv[]) {
    struct sockaddr_in saadr, caddr;
    int buff_size = atoi(argv[2]), handle, size = 10000*1024, counter = 0, n = 0;
    char *buffer = (char*) malloc(buff_size*sizeof(char));
    memset(&saadr, 0, sizeof(saadr));
    memset(&caddr, 0, sizeof(caddr));
    saadr.sin_family = AF_INET;
    saadr.sin_addr.s_addr = htonl(INADDR_ANY);
    saadr.sin_port = htons(atoi(argv[1]));
    socklen_t clen = sizeof(caddr);
    socklen_t addr_size;
    handle = socket(AF_INET, SOCK_DGRAM, 0);
    bind (handle, (struct sockaddr *) & saadr, sizeof(saadr));
    listen(handle, 2);
    setsockopt(handle, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
    recvfrom(handle, buffer, buff_size, 0, (struct sockaddr *)&caddr, &clen);
    file = fopen(buffer, "wb");
    do {        
        n = recvfrom(handle, buffer, buff_size, 0, (struct sockaddr *)&caddr, &clen);
        fwrite(buffer, sizeof(char), n, file);
        counter ++;
    } while (n == buff_size);
    fclose(file);
    close(handle);
    printf("Odebrano %d pakietow o wielkosci %d\n",counter, buff_size);
    return 0;
}