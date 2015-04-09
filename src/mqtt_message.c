#include <stdlib.h>

#include "mqqtt_message.h"
#include "server.h"
#include "util.h"
static mqtt_hash_t *topic_table;

struct mqtt_hash_t *get_topic_table()
{

    if(!topic_table)
    {
        struct server_env *env = get_server_env();
        if(!env) return NULL;
        return env->topic_table;
    }
    return topic_table;

} 
struct mqtt_topic *mqtt_topic_get(struct mqtt_string topic_name)
{
    struct mqtt_hash_t *_topic_table = get_topic_table();
    if(!_topic_table)
    {
        return NULL;
    }
    struct mqtt_hash_n *node = mqtt_hash_get(topic_table, topic_name);
    if(!node)
    {
        return NULL;
    }else{
        return (struct mqtt_topic *)node->data->body;
    }

}
struct mqtt_topic *mqtt_topic_init(struct mqtt_string topic_name)
{
    struct mqtt_topic *topic = malloc(sizeof(struct mqtt_topic));
    assert(topic);
    
    mqtt_string_copy(&topic_name, topic->name);
    topic->clients = NULL;
    topic->msg_sd_list->head = topic->msg_sd_list->tail = NULL;
    topic->msg_bf_list->head = topic->msg_bf_list->tail = NULL;
    return topic;
}

int mqtt_topic_add(struct mqtt_string topic_name)
{
    struct mqtt_hash_t *_topic_table = get_topic_table();
    if(!_topic_table)
    {
        return -1;
    }else{
        struct mqtt_topic *topic = mqtt_topic_get(topic_name);
        if(!topic)
        {
            struct mqtt_topic *topic = mqtt_topic_init(topic_name);
            assert(topic);
            struct mqtt_string data;
            mqtt_string_alloc(&data, (uint8_t *)topic);
            mqtt_hash_set(_topic_table, topic_name, data);
        }
        return 0;
    }
}

int mqtt_topic_add_msg(struct mqtt_string topic_name, struct mqtt_string msg)
{
    struct mqtt_topic topic = mqtt_topic_get(topic_name);
    if(!topic)
    {
        topic = mqtt_topic_add(topic_name);
        if(!topic)
        {
            printf("func topic_add_msg add topic failure\n");
            exit(1);
        }
    }

    if(_mqtt_topic_add_msg(topic, msg))
    {
        return 0;
    }else{
        return -1;
    }

} 

int _mqtt_topic_add_msg(struct mqtt_topic *topic, struct mqtt_string msg)
{
    if(!topic)
    {
        printf("func _mqtt_topic_add_msg topic NULL \n");
        return -1;
    } 
    struct msg_node *msg_n = mqtt_msg_init(msg);
    if(topic->msg_bf_list->head == NULL)
    {
       topic->msg_bf_list->head = topic->msg_bf_list->tail =  msg_n;
    }else{
        topic->msg_bf_list->tail->next = msg_n;
        topic->msg_bf_list->tail = msg_n;
    }
    if(topic->clients->next != NULL)
    {
        struct client_node *node = topic->clients->next;
                
    }
     
    return 0;
}

int mqtt_distribute_msg(struct client_in_hash *client_n, struct msg_node *msg_n)
{
    if(!client_n || !msg_n) return -1;
    if(client_n->head_nsend == NULL)  
    {
        struct msg_node *new_node = malloc(sizeof(struct msg_node));
        memset(
    }
    return 0;
}

struct msg_node *mgtt_msg_init(struct mqtt_string msg)
{
    struct msg_node *msg_n = malloc(sizeof(struct msg_node));
    aseert(msg_n);
    mqtt_string_copy(&msg, msg_n->body);
    msg_n->msg_id = mqtt_msg_id_gen();
    return msg_n;     
}

uint8_t *mqtt_msg_id_gen()
{
    int index = 0;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint8_t *id = malloc(sizeof(uint8_t) *17); 
    int tv_sec = tv.tv_sec;
    int tv_usec = tv.tv_usec;
    while(tv_sec > 0)
    {
        int byte = tv_sec % 10;
        id[index++] = byte;
        tv_sec /= 10;
    }
    long tv_usec = tv.tv_usec;
    while(index < 16)
    {
        int byte = tv_usec % 10;
        id[index++] = byte;
        tv_usec /= 10;
    }
    id[index] = '\0';
    return id;
}
