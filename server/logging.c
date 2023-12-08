#ifndef LOGGING
#define LOGGING
#include <syslog.h>
#include <sys/socket.h>

#define INFO 0 
#define WARNING 1
#define ERROR 2
#define RESPOND 3

void log_info(int type, const char* text) {
    switch (type) {
        case INFO:
            syslog(LOG_INFO, text);
            break;
        case WARNING:
            syslog(LOG_WARNING, text);
            break;
        case ERROR:
            syslog(LOG_CRIT, text);
            break;    
        case RESPOND:
            break;  
        default:
            break;
    }
}
#endif