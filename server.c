#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_NAME "/home/cheemsburger/Desktop/TESTF"
#define BUFFER_SIZE 128

int main(void) {
    struct sockaddr_un name;
    int ret;
    int connection_socket;
    int data_socket;
    int result;
    char buffer[BUFFER_SIZE];

    unlink(SOCKET_NAME);

    connection_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (connection_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&name, 0, sizeof(struct sockaddr_un));
    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, SOCKET_NAME, sizeof(name.sun_path) - 1);

    ret = bind(connection_socket, (const struct sockaddr *) &name, sizeof(struct sockaddr_un));
    if (ret == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    ret = listen(connection_socket, 20);
    if (ret == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        data_socket = accept(connection_socket, NULL, NULL);
        result = 0;
        for (;;) {
            memset(buffer, 0, BUFFER_SIZE);
            ret = read(data_socket, buffer, BUFFER_SIZE);
            if (ret == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }
            sprintf(buffer, "%s",  buffer);
            // snprintf(buffer, sizeof(buffer), "%d", result);
            ret = write(data_socket, buffer, strlen(buffer) + 1);
            if (ret == -1) {
                perror("write");
                exit(EXIT_FAILURE);
            }
            if (!strncmp(buffer, "DONE", strlen("DONE")))
                break;
            }
        }
    close(connection_socket);
    unlink(SOCKET_NAME);
    return 0;

    }
