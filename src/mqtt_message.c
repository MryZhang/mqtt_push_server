#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include "mqtt_message.h"
#include "server.h"
#include "util.h"
#include "mqtt_packet.h"
#include "mqtt3_protocal.h"
#include "log.h"
static struct mqtt_hash_t *topic_table;

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
    void *node = mqtt_hash_get(_topic_table, topic_name);
    if(node == NULL)
    {
        return NULL;
    }else{
        return (struct mqtt_topic *)node;
    }

}
struct mqtt_topic *mqtt_topic_init(struct mqtt_string topic_name)
{
    struct mqtt_topic *topic = malloc(sizeof(struct mqtt_topic));
    assert(topic);
    printf("topic mem add %d\n", topic); 
    memset(topic, '\0', sizeof(struct mqtt_topic)); 
    mqtt_string_copy(&topic_name, &(topic->name));
    LOG_PRINT("In function mqtt_topic_init:");
    LOG_PRINT("Init the topic with name [%s]", topic_name.body);
    LOG_PRINT("and new topic name [%s]", topic->name.body);
    return topic;
}

int mqtt_topic_add(struct mqtt_string topic_name, struct mqtt_topic **t)
{
    struct mqtt_hash_t *_topic_table = get_topic_table();
    if(!_topic_table)
    {
        return -1;
    }else{
        struct mqtt_topic *topic = mqtt_topic_get(topic_name);
        if(!topic)
        {
            LOG_PRINT("B");
            struct mqtt_topic *topic_init = mqtt_topic_init(topic_name);
            assert(topic_init);
            *t =topic_init;
            mqtt_hash_set(_topic_table, topic_name, (void*)topic_init);
        }else{
            LOG_PRINT("BBB");
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
        return -1;
    } 
    LOG_PRINT("A");
    struct topic_msg *msg_n = mqtt_topic_msg_init(msg);
    assert(msg_n != NULL);
    if(topic->msg_bf_list == NULL)
    {
        LOG_PRINT("ccc");
        topic->msg_bf_list = malloc(sizeof(struct msg_list));
        assert(topic->msg_bf_list);
        memset(topic->msg_bf_list, '\0', sizeof(struct msg_list));
        topic->msg_bf_list->head = msg_n;
        topic->msg_bf_list->tail = msg_n;
    }else{
        LOG_PRINT("ddd");
        topic->msg_bf_list->tail->next = msg_n;
        topic->msg_bf_list->tail = msg_n; 
    }
    LOG_PRINT("A");
    struct client_node *node = topic->clients_head;
    LOG_PRINT("A");
    while(node != NULL)
    {
        mqtt_distribute_msg(node->pclient, topic, msg_n);
        node = node->next;
    }
     
    LOG_PRINT("A");
    return 0;
}
struct topic_msg * mqtt_topic_msg_init(struct mqtt_string msg)
{
    struct topic_msg *t_msg = malloc(sizeof(struct topic_msg));
    assert(t_msg != NULL);
    memset(t_msg, '\0', sizeof(struct topic_msg));
    mqtt_string_copy(&msg, &(t_msg->message));
    t_msg->next = NULL;
    return t_msg;
}
//distribute the message to the clients which subscribe the topic
int mqtt_distribute_msg(struct client_in_hash *client_n, struct mqtt_topic *topic, struct topic_msg *msg_n)
{
    assert(client_n != NULL && msg_n != NULL);
    struct msg_node *p_msg_node = mqtt_msg_init(topic, msg_n->message.body);
    if(client_n->head_nsend == NULL)
    {
        client_n->head_nsend = client_n->tail_nsend = p_msg_node;
    }else{
        client_n->tail_nsend->next = p_msg_node;
        client_n->tail_nsend = p_msg_node;
    }
    if(client_n->sockfd > 0)
    {
        LOG_PRINT("In function mqtt_distribute_msg");
        LOG_PRINT("Send publish message to the client");
        mqtt_send_client_msg(client_n->sockfd);
    }
    
    LOG_PRINT("A");
    return 0;
}

struct msg_node *mqtt_msg_init(struct mqtt_topic *topic, uint8_t *publish_content)
{
    struct msg_node *client_node = malloc(sizeof(struct msg_node));
    assert(client_node);
    memset(client_node, '\0', sizeof(struct msg_node));
    struct mqtt_packet *packet = malloc(sizeof(struct mqtt_packet));
    assert(packet);
    memset(packet, '\0', sizeof(struct mqtt_packet));
    packet->command = PUBLISH;
    packet->topic = topic->name.body;
    packet->qosflag = 0;
    packet->dupflag = 0;
    packet->retainflag = 0;   
    packet->publish_content = publish_content;
    client_node->packet = packet;
    client_node->next = NULL;
    return client_node;
}

static int msg_id = 0;

int mqtt_msg_id_gen()
{
    msg_id = msg_id % 65535;
    return msg_id++;
}

int mqtt_topic_sub(struct mqtt_topic *topic, uint8_t *client_id, uint8_t qosflag)
{
    struct mqtt_string s_client_id;
    mqtt_string_alloc(&s_client_id, client_id, strlen(client_id));
    struct client_in_hash *client = mqtt_get_client_s(s_client_id); 
    if(!client)
    {
        LOG_PRINT("client in hash is null");
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
    LOG_PRINT("client [%s] has subscribed the topic [%s]", client_id, topic->name.body);
    return 0;
}

int mqtt_topic_unsub(struct mqtt_topic *topic, uint8_t* client_id)
{
    struct mqtt_string s_client_id;
    mqtt_string_alloc(&s_client_id, client_id, strlen(client_id));
    struct client_in_hash *client = mqtt_get_client_s(s_client_id);
    if(!client)
    {
        return -1;
    }    

    struct client_node *c_node;

    if(topic->clients_head == NULL && topic->clients_tail == NULL)
    {
        return 0;
    }else{
        c_node = topic->clients_head;
        int ret = 0;
        if(!mqtt_string_cmp(c_node->pclient->client_id, s_client_id))
        {
            topic->clients_head = c_node->next;
            c_node = NULL;
            return 0;
        }else{
            while(c_node->next != NULL)
            {
                struct client_in_hash *h_node = c_node->next->pclient;
                if(!mqtt_string_cmp(h_node->client_id, s_client_id))
                {
                    ret = 1;
                    break;
                }else{
                    c_node = c_node->next;
                }
            }
            if(ret == 1)
            {
                struct client_node *tmp = c_node->next;
                c_node->next = c_node->next->next;            
                tmp = NULL;
            }else{
            } 
            return 0;
        }
    }
    return 0;
}

