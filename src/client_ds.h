#ifndef _CLIENT_COM_H_
#define _CLIENT_COM_H_
#include <time.h>
#include <netinet/in.h>
#include "mqtt_timer.h"
#include "mqtt_string.h"

struct client_data
{
    struct util_timer timer;    
    int sockfd;
    struct sockaddr_in address;
    void (*dead_clean)(int sockfd);  // the callback function used to shutdown the connection which is unactivitive for a long time
    uint8_t *client_id;
};

struct client_in_hash
{
    int sockfd;
    struct mqtt_string client_id;
    struct client_msg_node *head_nsend;  
    struct client_msg_node *tail_nsend;
};
struct client_node
{
    struct mqtt_string id;
    struct client_node *next;
    struct client_in_hash *pclient;
};


#endif
