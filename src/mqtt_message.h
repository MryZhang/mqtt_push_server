#ifndef _MQTT_MESSAGE_H_
#define _MQTT_MESSAGE_H_

#include <stdint.h>
#include "mqtt_string.h"

struct msg_node
{
    struct mqtt_string *body;
    uint8_t *msg_id
    uint8_t f_retain;
    uint8_t f_send;
};
struct mqtt_topic
{
    struct mqtt_string name;
    struct client_node *clients;
    struct msg_node *msg_head;
    struct msg_node *msg_buff_head;
};
struct client_msg_node
{
    struct mqtt_string msg_id; 
    int f_send;
    struct mqtt_string *msg_string
};

#endif
