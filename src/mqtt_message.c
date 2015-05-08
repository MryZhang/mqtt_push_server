#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "mqtt_message.h"
#include "server.h"
#include "util.h"
static struct mqtt_hash_t *topic_table;

struct mqtt_hash_t *get_topic_table()
{
   // printf("a\n");
    if(!topic_table)
    {
       // printf("b\n");
        struct server_env *env = get_server_env();
    //printf("c\n");
        if(!env) return NULL;
    //printf("d\n");
        return env->topic_table;
    }
    //printf("e\n");
    return topic_table;

} 
struct mqtt_topic *mqtt_topic_get(struct mqtt_string topic_name)
{
    struct mqtt_hash_t *_topic_table = get_topic_table();
    if(!_topic_table)
    {
        printf("Err: topic_table is NULL POINTER in mqtt_topic_get\n");
        return NULL;
    }
    struct mqtt_hash_n *node = mqtt_hash_get(_topic_table, topic_name);
    if(!node)
    {
        printf("Err: the topic [%s] is not existed\n", topic_name.body);
        return NULL;
    }else{
        printf("Info: get the topic[%s] in mqtt_topic_get\n", topic_name.body);
        return (struct mqtt_topic *)node->data.body;
    }

}
struct mqtt_topic *mqtt_topic_init(struct mqtt_string topic_name)
{
    struct mqtt_topic *topic = malloc(sizeof(struct mqtt_topic));
    assert(topic);
    
    mqtt_string_copy(&topic_name, &(topic->name));
    topic->msg_sd_list.head = topic->msg_sd_list.tail = NULL;
    topic->msg_bf_list.head = topic->msg_bf_list.tail = NULL;
    return topic;
}

int mqtt_topic_add(struct mqtt_string topic_name, struct mqtt_topic **t)
{
    printf("Info:  add the topic [%s]\n", topic_name.body);
    struct mqtt_hash_t *_topic_table = get_topic_table();
    //printf("1\n");
    if(!_topic_table)
    {
        return -1;
    }else{
        //printf("2\n");
        struct mqtt_topic *topic = mqtt_topic_get(topic_name);
        if(!topic)
        {
            printf("Info: you can add the topic [%s]\n", topic_name.body);
            struct mqtt_topic *topic = mqtt_topic_init(topic_name);
            assert(topic);
            printf("Info: topic name [%s] in mqtt_topic_add\n", topic->name.body);
            struct mqtt_string data;
            if(t != NULL)
            {
                *t = topic;
            }
            mqtt_string_alloc(&data, (uint8_t *)topic, sizeof(struct mqtt_topic));
            struct mqtt_topic *tt = (struct mqtt_topic *) (data.body);
            printf("Info: tt topic [%s]\n", tt->name.body);
            mqtt_hash_set(_topic_table, topic_name, data);
        }
        return 0;
    }
}

int mqtt_topic_add_msg(struct mqtt_string topic_name, struct mqtt_string msg)
{
    struct mqtt_topic *topic = mqtt_topic_get(topic_name);
    if(!topic)
    {
        int ret = mqtt_topic_add(topic_name, &topic);
        if(!ret)
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
        printf("Err: need a topic to add msg\n");
        return -1;
    } 
    struct msg_node *msg_n = mqtt_msg_init(msg);
    if(topic->msg_bf_list.head == NULL)
    {
       topic->msg_bf_list.head = topic->msg_bf_list.tail =  msg_n;
    }else{
        topic->msg_bf_list.tail->next = msg_n;
        topic->msg_bf_list.tail = msg_n;
    }
    struct client_node *node = topic->clients_head;
    while(node != NULL)
    {
        mqtt_distribute_msg(node->pclient, msg_n);
        node = node->next;
    }
     
    printf("Info: add msg [%s] to the topic [%s]\n", msg.body, topic->name.body);
    return 0;
}
//distribute the message to the clients which subscribe the topic
int mqtt_distribute_msg(struct client_in_hash *client_n, struct msg_node *msg_n)
{
    if(!client_n || !msg_n) return -1;
    //copy the message
    struct client_msg_node *cnode = mqtt_client_msg_init(msg_n->body);
    assert(cnode); 
    if(client_n->head_nsend == NULL)  
    {
        client_n->head_nsend = client_n->tail_nsend = cnode;    
    }else{
        cnode->next = client_n->head_nsend;
        client_n->head_nsend = cnode;
    }
    return 0;
}

struct msg_node *mqtt_msg_init(struct mqtt_string msg)
{
    struct msg_node *msg_n = malloc(sizeof(struct msg_node));
    assert(msg_n);
    mqtt_string_copy(&msg, &(msg_n->body));
    msg_n->msg_id = mqtt_msg_id_gen();
    return msg_n;     
}

struct client_msg_node *mqtt_client_msg_init(struct mqtt_string msg)
{
    struct client_msg_node *node = malloc(sizeof(struct client_msg_node));
    mqtt_string_alloc(&(node->msg_id), msg.body, msg.len);
    node->f_send = 0;
    node->next = NULL;
    return node;
}
uint8_t *mqtt_msg_id_gen()
{
    int index = 0;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint8_t *id = malloc(sizeof(uint8_t) *17); 
    int tv_sec = tv.tv_sec;
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

int mqtt_topic_sub(struct mqtt_topic *topic, uint8_t *client_id, uint8_t qosflag)
{
    struct mqtt_string s_client_id;
    mqtt_string_alloc(&s_client_id, client_id, strlen(client_id));
    struct client_in_hash *client = mqtt_get_client_s(s_client_id); 
    if(!client)
    {
        printf("Error: could not find the client\n");
        return -1;
    } 
    struct client_node *c_node = malloc(sizeof(struct client_node));
    assert(c_node);
    memset(c_node, '\0', sizeof(struct client_node));    
    c_node->next = NULL;
    c_node->pclient = client; 
    c_node->qos = qosflag;

    if(topic->clients_head == NULL || topic->clients_tail == NULL)
    {
        topic->clients_head = topic->clients_tail = c_node;
    }else{
        topic->clients_tail->next = c_node;
        topic->clients_tail = c_node; 
    }
    return 0;
}



