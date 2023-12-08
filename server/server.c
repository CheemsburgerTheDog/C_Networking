#include "network.c"
#include "s_network.c"
#define TCP_C_SIZE 5
#define UDP_C_SIZE 10
#define CAP 10

int main() {
    InitServer("127.0.0.1", 7888, TCP_C_SIZE, 7889, UDP_C_SIZE, CAP);
    Start();
}