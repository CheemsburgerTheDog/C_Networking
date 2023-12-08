#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <poll.h>

FILE *file;
struct dzialanie {
    int a;
    int b;
};
// 7777
int main (int argc, char* argv[]) {
    struct pollfd fds[2];
    struct dzialanie received;
    struct sockaddr_in udp_saddr, tcp_saddr;
    struct sockaddr_in udp_caddr, tcp_caddr;
    socklen_t udp_clen, tcp_clen;
    int udp_handle, tcp_sandle, tcp_chandle = 0, result = 0, retval = 0;

    memset(&udp_saddr, 0, sizeof(udp_saddr));
    memset(&tcp_saddr, 0, sizeof(tcp_saddr));
    memset(&udp_caddr, 0, sizeof(udp_caddr));
    memset(&tcp_caddr, 0, sizeof(tcp_caddr));

    udp_saddr.sin_family = AF_INET;
    udp_saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_saddr.sin_port = htons(7778);
    udp_clen = sizeof(udp_caddr);
    udp_handle = socket(AF_INET, SOCK_DGRAM, 0);
    bind (udp_handle, (struct sockaddr *) & udp_saddr, sizeof(udp_saddr));
    listen(udp_handle, 2);

    tcp_saddr.sin_family = AF_INET;
    tcp_saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcp_saddr.sin_port = htons(7777);
    tcp_clen = sizeof(tcp_caddr);
    tcp_sandle = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(tcp_sandle, F_SETFL, fcntl(tcp_sandle, F_GETFL, 0) | O_NONBLOCK);
    bind (tcp_sandle, (struct sockaddr *) & tcp_saddr, sizeof(tcp_saddr));
    listen(tcp_sandle, 2);

    fds[0].fd = udp_handle;
    fds[0].events = POLLIN;
    fds[1].fd = tcp_chandle;
    fds[1].events = POLLIN;
    
    while (1)
    {   
        tcp_chandle = accept(tcp_sandle, (struct sockaddr *) &tcp_caddr, &tcp_clen);
        fds[1].fd = tcp_chandle;
        if (tcp_chandle == -1) { retval = poll(fds, 2, 0); }
        else { 
            retval = poll(fds, 2 , 10000);
            if (!retval) {
                printf("TCP Timeout!");
                fflush(stdout);
                close(tcp_chandle);
            }
            if (fds[1].revents & POLLIN) {
                recv(tcp_chandle, &received, sizeof(struct dzialanie) , 0 ); 
                result = received.a+received.b;
                send(tcp_chandle, &result, sizeof(int), 0);
                close(tcp_chandle);
            }
        }
        if (fds[0].revents & POLLIN) {
            recvfrom(udp_handle, &received, sizeof(struct dzialanie), 0, (struct sockaddr *)&udp_caddr, &udp_clen);
            result = received.a + received.b;
            sendto(udp_handle, &result, sizeof(int), 0 , (struct sockaddr *) &udp_caddr, udp_clen);
        }
        fflush(stdout);
    }
    return 0;





        // if (tcp_chandle != -1) {

        //     struct dzialanie received;
        //     while ( 0 > recv(tcp_chandle, &received, sizeof(struct dzialanie) ,0 ) ){
        //         continue;
        //     }
        //     result = received.a+received.b;
        //     send(tcp_chandle, &result, sizeof(int), 0);
        //     close(tcp_chandle);
        // }
        // if (n = recvfrom(handle, buffer, buff_size, 0, (struct sockaddr *)&caddr, &clen);
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