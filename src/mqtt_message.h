#ifndef _MQTT_MESSAGE_H_
#define _MQTT_MESSAGE_H_

#include <stdint.h>
#include "mqtt_string.h"
#include "client_ds.h"

struct msg_node
{
    struct mqtt_packet *packet;
    struct msg_node *next;
};
struct topic_msg
{
    struct mqtt_string message;
    struct topic_msg *next;
};
struct msg_list
{
    struct topic_msg *head;
    struct topic_msg *tail;
};
struct mqtt_topic
{
    struct mqtt_string name;
    struct client_node *clients_head;
    struct client_node *clients_tail;
    struct msg_list msg_sd_list;
    struct msg_list msg_bf_list;
};

struct mqtt_hash_t *get_topic_table();
struct mqtt_topic *mqtt_topic_get(struct mqtt_string topic_name);
struct mqtt_topic *mqtt_topic_init(struct mqtt_string topic_name);
int mqtt_topic_add(struct mqtt_string topic_name, struct mqtt_topic **t);
int mqtt_topic_add_msg(struct mqtt_string topic_namne, struct mqtt_string msg);
int _mqtt_topic_add_msg(struct mqtt_topic *topic, struct mqtt_string msg);
struct msg_node *mqtt_msg_init(struct mqtt_topic *topic, uint8_t *publish_content);
struct topic_msg *mqtt_topic_msg_init(struct mqtt_string msg);
int mqtt_msg_id_gen();
int mqtt_topic_sub(struct mqtt_topic *topic, uint8_t *client_id, uint8_t qosflag);
#endif
