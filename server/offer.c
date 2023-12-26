#ifndef OFFER
#define OFFER
#include "s_network.c"
#include "network.c" 
int g_offer_id = 1;

void receive_new();
void propagate();
void accept_supplier(int, Message*, Offer*, int, pthread_mutex_t*);
void _gen_offer_id();
void receive_new(int connection, Message *msg, Offer *olist, int no, User *ulist, int nu, pthread_mutex_t *offer_mut) {
    pthread_mutex_lock(offer_mut);
    for (size_t i = 0; i < no; i++) {
        if (olist[i].phase == 2 ) {
            if (send_(connection, NEW_ACCEPTED, NULL) == -1) { 
                pthread_mutex_unlock(offer_mut);
                return; 
            }
            olist[i].phase = 0;
            int max = 0;
            for (size_t d = 0; d < no; d++) {
                if (max <= olist[d].id) { max = olist[d].id; }
            }
            olist[i].id = max+1;
            pthread_mutex_unlock(offer_mut);
            //PESPO 3500 DREWNO 15
            sscanf(msg->message,"%s %d %s %d", olist[i].client_name, &olist[i].eta, olist[i].resource, &olist[i].quantity);
            olist[i].cli_handle = connection;
            for (size_t i = 0; i < nu; i++) {
                if (ulist[i].handle == connection) {
                    ulist[i].busy = true;
                }
            }
            propagate(msg, ulist, nu);
            return;
        }
    }
    pthread_mutex_unlock(offer_mut);
    send_(connection, NEW_DECLINED, NULL);
    return;
}
// void request_all(int connection, Offer *list, int n) {
//     Message msg;
//     msg.code = NEW_OFFER;
//     for (size_t i = 0; i < n; i++) {
//         if (list[i].done == 0) {
            
//         }
//     }
    
// }
// void accept_supplier(int connection, Message *msg, Offer *list, int ln, pthread_mutex_t *mut) {
//     int id;
//     sscanf(msg->message, "%d", &id);
//     pthread_mutex_lock(mut);
//     for (size_t i = 0; i < ln; i++) {
//         if (list[i].id == id) {
//             if (list[i].phase == (1||2)) {
//                 send_(connection, ACCEPT_DECLINE, NULL);
//             } else {
//                 if (send_(connection, ACCEPT_ACCEPT , msg) == -1) {
//                     pthread_mutex_unlock(mut);
//                     return;
//                 } else {
//                     list[i].phase = 1;
//                     pthread_mutex_unlock(mut);
//                     return;
//                     //VIP ustawic pedalow na BUSY
//                 }
//             }
//         }    
//     }
// }

// void finilize_offer(int connection, Message *msg, Offer *olist, int on, pthread_mutex_t *omut, User *ulist, int un, pthread_mutex_t *umut) {
//     int id;
//     sscanf(msg->message, "%d", &id);
//     pthread_mutex_lock(omut);
    



// }
    
inline void propagate(Message *msg, User *list, int n) {
    for (size_t i = 0; i < n; i++) {
        if (list[i].type == SUPPLIER && list[i].active == true && list[i].busy == false) { 
            send_(list[i].handle, NEW_OFFER, msg); 
        }    
    }
}


#endif
