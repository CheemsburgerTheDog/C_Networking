#include "network.c"
#include "s_network.c"
#define CLIENTS_SIZE 5
#define PORT 7030
#define THREADS 1
#define USER_CAP 10
#define OFFER_CAP 10

int main() {
    InitPasswd("/home/cheemsburger/Desktop/C_Networking/server/passwd");
    InitServer(PORT, CLIENTS_SIZE, THREADS, USER_CAP, OFFER_CAP);
}