#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_NAME "/home/cheemsburger/Desktop/TESTF"
#define BUFFER_SIZE 128

int main(void) {
    struct sockaddr_un addr;
    int i;
    int ret;
    int data_socket;
    char buffer[BUFFER_SIZE];

    data_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (data_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);

    ret = connect(data_socket, (const struct sockaddr *) &addr, sizeof(struct sockaddr_un));
    if (ret == -1) {
        fprintf(stderr, "The server is down.\n");
        exit(EXIT_FAILURE);
    }

    do {
        printf("Enter number to send to server :\n");
        scanf("%d", &i);
        sprintf(buffer, "%d", i);
        ret = write(data_socket, buffer, strlen(buffer) + 1);
        if (ret == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        printf("Number sent to server, waiting for result...\n");

        memset(buffer, 0, BUFFER_SIZE);
        ret = read(data_socket, buffer, BUFFER_SIZE);
        if (ret == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        printf("Received from server: %s\n", buffer);

    } while (strncmp(buffer, "Result =", strlen("Result =")));

    close(data_socket);

    return 0;
}
