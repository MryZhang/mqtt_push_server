#ifndef _MQTT_MESSAGE_H_
#define _MQTT_MESSAGE_H_

#include <stdint.h>
#include "mqtt_string.h"
#include "client_ds.h"

struct msg_node
{
    struct mqtt_string body;
    uint8_t *msg_id;
    uint8_t f_retain;
    uint8_t f_send;
    struct msg_node *next;
};
struct msg_list
{
    struct msg_node *head;
    struct msg_node *tail;
};
struct mqtt_topic
{
    struct mqtt_string name;
    struct client_node clients;
    struct msg_list msg_sd_list;
    struct msg_list msg_bf_list;
};
struct client_msg_node
{
    struct mqtt_string msg_id; 
    int f_send;
    struct mqtt_string *msg_string;
    struct client_msg_node *next;
};

struct mqtt_hash_t *get_topic_table();
struct mqtt_topic *mqtt_topic_get(struct mqtt_string topic_name);
struct mqtt_topic *mqtt_topic_init(struct mqtt_string topic_name);
int mqtt_topic_add(struct mqtt_string topic_name, struct mqtt_topic **t);
int mqtt_topic_add_msg(struct mqtt_string topic_namne, struct mqtt_string msg);
int _mqtt_topic_add_msg(struct mqtt_topic *topic, struct mqtt_string msg);
struct msg_node *mqtt_msg_init(struct mqtt_string msg);
struct client_msg_node *mqtt_client_msg_init(struct mqtt_string msg);
uint8_t *mqtt_msg_id_gen();
#endif
