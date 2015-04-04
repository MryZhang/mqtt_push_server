#ifndef _CLIENT_COM_H_
#define _CLIENT_COM_H_
#include <time.h>
#include <netinet/in.h>
#include "mqtt_timer.h"

struct client_data
{
    struct sockaddr_in address;
    int sockfd;
    struct util_timer *timer;    
    void (*dead_clean)(int sockfd);  // the callback function used to shutdown the connection which is unactivitive for a long time
};


#endif
