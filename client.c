
#include <arpa/inet.h> 
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> 
#include <sys/socket.h>
#include <unistd.h> 
#include <stdbool.h>
#include <poll.h>
#include <string.h>

#define LISTSIZE 10
#define BUFFSIZE 50
#define PORT 7777
#define SA struct sockaddr


typedef struct {
    int offer_id;
    char item[BUFFSIZE];
    int time;
    bool is_active;
} offer;

typedef struct {

    int code;
    char message[BUFFSIZE];
} message;
bool timer_paused = false;
int user_mode = 0;
int handle;
struct pollfd fds[2];
offer offer_list[LISTSIZE];

void add_offer(message msg) {
    int my_index = 0;
    for (int i = 0; i<LISTSIZE; i++) {
        if (offer_list[i].is_active == false) { my_index = i; }
    }
    char *token = strtok(msg.message, " ");
    offer_list[my_index].time = atoi(token);
    token = strtok(NULL," ");
    offer_list[my_index].offer_id = atoi(token);
    token = strtok(NULL,"");
    strncpy(offer_list[my_index].item, token, sizeof(offer_list[my_index].item));
}

void timer() {
    while (1) {
        system("clear");
        while (timer_paused) {}; //Block till stdin is being used. For STDOUT clarity;
        if (user_mode == 1) { printf("1. Accept offer\n2. Exit \n"); }
        else { printf("1. Publish offer\n2. Exit \n"); }
        for (int i = 0; i<LISTSIZE; i++) {
            if (offer_list[i].is_active == true) {
                printf("%d %s %d\n", offer_list[i].offer_id, offer_list[i].item, offer_list[i].time);
                offer_list[i].time -=1;
                if (offer_list[i].time <= 0) { offer_list[i].is_active = false; }
            } 
        }
        fflush(stdout);
        sleep(2);
    }
}
void client_mode() {
    // fds[0].fd = 0;    //STDIN
    // fds[0].events = POLLIN;
    // fds[1].fd = handle; // Socket
    // fds[1].events = POLLIN;
    while ( true ) {
        // if ( -1 == poll(fds, 2, 0) ){ exit(1); } // Not nesseccary. Synchronous IO
        // if (fds[0].revents & POLLIN) {
            int action  = 0;
            message msg;
            scanf("%d", &action);
            switch (action) {
                case 1: { //Send offer to the server
                    msg.code = 100; 
                    timer_paused = true;
                    printf("Enter offer details. Format TIMEOUT-RESOURCE:  ");
                    fflush(stdout);
                    scanf("%s", msg.message);
                    send(handle, &msg, sizeof(message), 0);
                    recv(handle, &msg, sizeof(message), 0); // Server result
                    switch (msg.code) {
                        case 150: { //Handle offer till the end
                            add_offer(msg); //Add offer to the timer 
                            timer_paused = false;
                            recv(handle, &msg, sizeof(message), 0); //Offer taken by the supplier
                            timer_paused = true;
                            recv(handle, &msg, sizeof(message), 0); // Offer completed. End the offer
                            break;
                        }
                        case 250: { //ERROR Offer declined
                            printf("Offer declined. Try again");
                            sleep(1);
                            break;
                        }
                        default:
                            break;    
                    }
                }
                case 2: { // User logout;
                    msg.code = 500;
                    send(handle, &msg, sizeof(message), 0);
                    close(handle);
                    exit(1); 
                }
                default : { break; }
            // }
        }
    }
}




//         for ( int i = 0; i < 2; i++ ) {
//             if ( fds[i].revents & POLLIN ) {
//                 if (i == 0) { // Input STDIN
//                     char user_input[20];
//                     int n = read(0, user_input, sizeof(user_input));
//                     if ( n>0 ) { user_input[n] = '/0'; }
//                     switch ( atoi(user_input) ) {
//                         case 1: // Send new offer to the server
//                         message msg;
//                         msg.code = 120;
//                         printf("Podaj towar na ktory jest zapotrzebowanie w formacie CZAS-ZASOB: ");
//                         scanf("%s", msg.message);
//                         send(handle, &msg, sizeof(message), 0);
//                         break;
//                         case 2: //Close the program
//                             close(handle);
//                             exit(0);
//                     }
//                 }
//                 else {
//                     //ODCZYT WIADOMOSCI Z SERWERA
//                 }   
// }   }   }   }

void supplier_mode(int handle) { 

    //pt_thread_create(timer);
    fds[0].fd = 0;    //Set descriptors
    fds[0].events = POLLIN;
    fds[1].fd = handle;
    fds[1].events = POLLIN;

    while (1) {
        if ( -1 == poll(fds, 2, -1) ) {  // Poll descriptor exit on failure
            close(handle);
            exit(1); 
        }
        if (fds[0].revents & POLLIN) {
            
        }
        



        for (int i = 0; i < 2; i++) { 
            if ( fds[i].revents & POLLIN ) {
                if (i == 0) { // Input STDIN
                    char user_input[20];
                    int n = read(0, user_input, sizeof(user_input));
                    if (n>0) {user_input[n] = '/0';}
                    switch (atoi(user_input)) {
                    case 1: //Send offer_id to be taken
                        message to_send;
                        to_send.code = 200;
                        scanf("%d", &index);
                        sprintf(to_send.message, "%ld", index);
                        send(handle, &to_send, sizeof(message), 0);
                        break;
                    case 2: //Close the program
                        close(handle);
                        exit(0);
                    }
                } else if (i == 1) { // Input from the socket
                    message mess;
                    int n = read(handle, &mess, sizeof(message));
                    switch (mess.code) {
                        case 11: //NEW_OFFER
                            add_offer(mess);
                            break;
                        case 12: //OFFER_EXPIRED
                            printf("Oferta nr. %s wygasla", mess.message);
                                                        // SPRAWDZ ID Z TYMI NA LESCIE USTAW JA NA ACTIVE = FALSEL

                            break;
                        case 14: //CONNECTION_CLOSED
                            print("Serwer zakonczyl polaczenie. Zamykam aplikacje");
                            close(handle);
                            exit(1);
                            break;
                        case 13: // SET_BUSY_TRUE
                            printf("Uzytkownik zajety. Nie mozna przyjac dwoch zlecen jednoczesnie");
                            break;
}   }   }   }   }   }

int login(int handle) {
    message temp;
    bzero(&temp, sizeof(message));
    printf("Podaj dane do logowania: ");
    scanf("%s", temp.message);
    temp.code = 10;
    send(handle, &temp, sizeof(temp), 0);
    recv(handle, &temp, sizeof(temp), 0); 
    //Send message with credentials, await respond usermode
    // and act accordingly
    switch (temp.code) {
        case 10: //Invalid credentials
            return 1;
            break;
        case 11: //Client
            client_mode(1);
            return 0;
            break;
        case 12: //Supplier
            supplier_mode(1);
            return 0;
            break;
}   }
void setup_list();

int main() {
    setup_list(); //Prepare list with preset values;
    int sock; //Establish connection
    struct sockaddr_in servaddr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("socket creation failed...\n");
        exit(0);
    } else {
        printf("Socket successfully created..\n");
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);
    if (connect(sock, (SA*)&servaddr, sizeof(servaddr))!= 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
    int option = 0;
    printf("1. Register\n2.Login\n");
    scanf("%d", &option); //Login to the server
    switch (option) {
        case 2:
            login();
            break;
        case 1:
            reg();
            break;
    }
    close(sock);
    return 0;
}
void setup_list() {
    for (int i = 0; i<LISTSIZE; i++ ){
        offer_list[i].is_active = false;
        strncpy(offer_list[i].item, "NaN", 3);
        offer_list[i].time = 0;
        offer_list[i].offer_id = 0;
    }
}


