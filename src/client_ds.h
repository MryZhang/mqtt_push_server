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
};


#endif
