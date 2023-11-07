#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

FILE *file;
struct dzialanie {
    int a;
    int b;
};
// 7070 UDP 1024 name.mp4
int main (int argc, char* argv[]) {

    struct sockaddr_in udp_saddr, tcp_saddr;
    struct sockaddr_in udp_caddr, tcp_caddr;

    int udp_handle, tcp_handle, tcp_chandle;

    int buff_size = atoi(argv[3]);
    char *buffer = (char*) malloc(buff_size*sizeof(char));

    memset(&udp_saddr, 0, sizeof(udp_saddr));
    memset(&tcp_saddr, 0, sizeof(tcp_saddr));
    memset(&udp_caddr, 0, sizeof(udp_caddr));
    memset(&tcp_caddr, 0, sizeof(tcp_caddr));

    udp_saddr.sin_family = AF_INET;
    udp_saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_saddr.sin_port = htons(atoi(argv[1]));
    
    tcp_saddr.sin_family = AF_INET;
    tcp_saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcp_saddr.sin_port = htons(atoi(argv[1]));
    tcp_handle = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(tcp_handle, F_SETFL, fcntl(tcp_handle, F_GETFL, 0) | O_NONBLOCK);
    bind (tcp_handle, (struct sockaddr *) & tcp_saddr, sizeof(tcp_saddr));
    listen(tcp_handle, 2);
    while (1)
    {
        tcp_chandle = connect()
    }
    


    // socklen_t clen = sizeof(caddr);
    // socklen_t addr_size;

    // while (1)
    // if (strncmp(argv[2],"UDP", 3) == 0){
    //     mode = 0;
    //     handle = socket(AF_INET, SOCK_DGRAM, 0);
    //     bind (handle, (struct sockaddr *) & saadr, sizeof(saadr));
    //     listen(handle, 2);
    //     setsockopt(handle, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
    //     recvfrom(handle, buffer, buff_size, 0, (struct sockaddr *)&caddr, &clen);
    // } else if (strncmp(argv[2],"TCP", 3) == 0) {
    //     mode = 1;
    //     handle = socket(AF_INET, SOCK_STREAM, 0);
    //     bind (handle, (struct sockaddr *) & saadr, sizeof(saadr));
    //     setsockopt(handle, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
    //     listen(handle, 2);
    //     chandle = accept(handle, (struct sockaddr *) &caddr, &addr_size);
    //     sleep(1);
    //     recv(chandle, buffer, buff_size, 0);
    // } else { exit(1); }
    // file = fopen(buffer, "wb");
    // switch (mode) {
    // case 0:
    //     do {        
    //         n = recvfrom(handle, buffer, buff_size, 0, (struct sockaddr *)&caddr, &clen);
    //         fwrite(buffer, sizeof(char), n, file);
    //         counter ++;
    //     } while (n == buff_size);
    //     break;
    // case 1:
    //     do {        
    //         n = recv(chandle, buffer, buff_size, 0);
    //         fwrite(buffer, sizeof(char), n, file);
    //         counter ++;
    //     } while (n == buff_size);
    //     break;
    // }
    // fclose(file);
    // close(handle);
    // close(chandle);
    // printf("Odebrano %d pakietow o wielkosci %d\n",counter, buff_size);
    return 0;
}