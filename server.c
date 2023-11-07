#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <strings.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#define BUFFSIZE 30
#define USERSIZE 100
#define OFFERSIZE 100
FILE *file;

typedef struct {

    int code;
    char message[BUFFSIZE];
} message;

typedef struct {
    int offer_id;
    int client_id;
    int supplier_id;
    char item[BUFFSIZE];
    int time;
    bool is_active;
    bool is_done;
} offer;

typedef struct {
    int id;
    bool is_busy;
} user;
offer offers[OFFERSIZE];
user clients[USERSIZE];
user suppliers[USERSIZE];

void update_offers() {
    while (1) {
        for (int i = 0; i<OFFERSIZE; i++) {
            if (offers[i].is_active == true) { 
                offers[i].time -=1;
                if (offers[i].time <= 0) { 
                    offers[i].is_active = false; 
                }
            }
        } 
        sleep(2);
    }
}
void reopen_file() {
    fclose(file);
    file = fopen("userdata.txt", "a+");
}

void register_user (int handle, message msg) {
    char *token = strtok(msg.message, " ");
    char token_buffer[BUFFSIZE];
    char line_buffer[BUFFSIZE];
    bzero(token_buffer, sizeof(token_buffer));
    bzero(line_buffer, sizeof(line_buffer));
    strcpy(token, token_buffer);
    int counter = 0;
    reopen_file();
    bool is_found;
    while (fgets(line_buffer, sizeof(line_buffer), file)!=NULL) {
        if (counter%4 == 0) {
            for (int i = 0; i<sizeof(line_buffer); i++) {
                if( line_buffer[i]!=token[i] ) { break; }
            }
            }
        }
        message
        bzero(temp_buf, sizeof(temp_buf));
        counter++;
    }

    return 0;
}

void user_thread(int my_handle) {
    message temp;
    bzero(&temp, sizeof(message));
    recv(my_handle, &temp, sizeof(message), 0);
    switch ( temp.code ) {
    case 10:
        login();
    case 20:
        register_user(my_handle, &temp);
        break;

    
    default:
        break;
    }
}
int main(int argc, char **argv) {

    srand(time(NULL));
    file = fopen("userdata.txt", "a+"); // Initial file open

    int server_handle, client_handle; // Socket setup and initialization
    struct sockaddr_in server_addr;
    struct sockaddr_storage client_storage;
    socklen_t addr_size;
    bzero(&server_addr, sizeof(server_addr));
    bzero(&client_storage, sizeof(client_storage));
    server_handle = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(7777);
    bind(server_handle, (struct sockaddr *) &server_addr, sizeof(server_addr));
    listen(server_handle, 100);

    while(1){
        fflush(stdout);
        addr_size = sizeof(client_storage);
        client_handle = accept(server_handle, (struct sockaddr *) &client_storage, &addr_size);
        user_thread(client_handle);
        // pthread_create(&user_thread_list[user_count++], NULL, &user_thread, &client_handle);
        // pthread_join(user_thread_list[user_count], NULL);
    }
    return 0;

}
