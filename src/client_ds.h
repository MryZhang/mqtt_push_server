#ifndef _CLIENT_COM_H_
#define _CLIENT_COM_H_
#include <time.h>
#include <netinet/in.h>
#include "mqtt_timer.h"
#include "mqtt_string.h"
#include "mqtt_hash.h"

struct client_data
{
    struct util_timer timer;    
    int sockfd;
    struct sockaddr_in address;
    void (*dead_clean)(int sockfd);  // the callback function used to shutdown the connection which is unactivitive for a long time
    uint8_t *client_id; 
    struct client_in_hash *client_hash_node;
};

struct client_in_hash
{
    int sockfd;
    struct mqtt_string client_id;
    struct msg_node *head_nsend;  
    struct msg_node *tail_nsend;
};
struct client_node
{
    struct client_node *next;
    struct client_in_hash *pclient;
    uint8_t qos;
};

struct mqtt_hash_t *get_client_table();
struct client_in_hash *mqtt_get_client_s(struct mqtt_string client_id);
struct client_in_hash *_mqtt_get_client_s(struct mqtt_hash_t *hash_t, struct mqtt_string client_id);
int mqtt_add_client_s(struct mqtt_string client_id, int sockfd);
int _mqtt_add_client_s(struct mqtt_hash_t *hash_t, struct mqtt_string client_id, int sockfd);
struct client_in_hash *_mqtt_init_client_in_hash(struct mqtt_string client_id, int sockfd);

#endif
