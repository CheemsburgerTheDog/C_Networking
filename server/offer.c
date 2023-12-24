#ifndef OFFER
#define OFFER
#include "s_network.c"
#include "network.c"
void receive_new();
void propagate();

void receive_new(int connection, Message *msg, Offer *olist, int nl, User *ulist, int nu) {
    for (size_t i = 0; i < nl; i++) {
        if (olist[i].done == 1 ) {
            // 1203 PESPO 3500 DREWNO 15
            sscanf(msg->message,"%d %s %d %s %d",&olist[i].id, olist[i].client_name, &olist[i].eta, olist[i].resource, &olist[i].quantity);
            send_(connection, NEW_ACCEPTED, NULL);
            propagate(msg, ulist, nu);
            return;
        }
    }
    send_(connection, NEW_DECLINED, NULL);
}
void request_all(int connection, Offer *list, int n) {
    Message msg;
    msg.code = NEW_OFFER;
    for (size_t i = 0; i < n; i++) {
        if (list[i].done == 0) {
            
        }
    }
    
}
inline void propagate(Message *msg, User *list, int n) {
    for (size_t i = 0; i < n; i++) {
        if (list[i].type == SUPPLIER && list[i].active == true && list[i].busy == false) { 
            send_(list[i].handle, NEW_OFFER, msg); 
        }    
    }
}


#endif
