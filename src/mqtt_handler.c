#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "mqtt_handler.h"
#include "mqtt_packet.h"
#include "server.h"
#include "hiredis.h"
#include "client_ds.h"
#include "net.h"
#include "redis_com.h"
#include "mqtt_message.h"
#include "log.h"

/* handler the CONNECT message */
int mqtt_handler_connect(struct mqtt_packet *packet)
{
    int ret, i;
    int sockfd = packet->fd->sockfd;
    uint8_t byte;
    if((ret = mqtt_remain_length(packet)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }     
    if((ret = mqtt_read_payload(packet)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    //Read the Protocol Name which follow the remain_length;
    if((ret = mqtt_read_protocol_name(packet)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    
    if((ret = mqtt_read_protocol_version(packet)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    
    if((ret = mqtt_read_connect_flags(packet)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    LOG_PRINT("Flag f_uname [%d]", packet->conn_f.f_uname);
    LOG_PRINT("Flag f_pwd [%d]", packet->conn_f.f_pwd);
    LOG_PRINT("Flag f_will_retian [%d]", packet->conn_f.f_will_retain);
    LOG_PRINT("Flag f_will_qos [%d]", packet->conn_f.f_will_qos);
    LOG_PRINT("Flag f_will [%d]", packet->conn_f.f_will);
    LOG_PRINT("Flag f_clean [%d]", packet->conn_f.f_clean);

    if((ret = mqtt_read_livetimer(packet)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    LOG_PRINT("livetime [%d]", packet->alive_timer);
    if((ret = mqtt_str(packet, &packet->identifier)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }  
    LOG_PRINT("client identitfier [%s]", packet->identifier);
    int len = 0;
    while(packet->identifier[len] != '\0') len++;
    if(len > 23)
    {
        mqtt_conn_ack(packet, 2);
        return MQTT_ERR_ID_TOO_LONG;
    }
     
    if(!had_client_id(packet->identifier))
    {
        if((ret = add_client_id(packet->identifier)) != MQTT_ERR_SUCCESS)
        {
            return ret;
        }
    }else{
        return MQTT_ERR_ID_REJECTED;
    }


    if(packet->conn_f.f_will)
    {
        mqtt_console_payload(packet);
        if((ret = mqtt_str(packet, &packet->will_topic)) != MQTT_ERR_SUCCESS)
        {
            return ret;
        } 
        LOG_PRINT("Will Topic[%s]", packet->will_topic); 
        if((ret = mqtt_str(packet, &packet->will_message)) != MQTT_ERR_SUCCESS)
        {
            return ret;
        }
        LOG_PRINT("Will Message [%s]", packet->will_message);
    }
    if(packet->conn_f.f_uname)
    {
        if((ret = mqtt_str(packet, &packet->username)) != MQTT_ERR_SUCCESS)
        {
            return ret;
        }
        LOG_PRINT("auth username [%s]", packet->username);    
        if(packet->conn_f.f_pwd)
        {
            if((ret = mqtt_str(packet, &packet->password)) != MQTT_ERR_SUCCESS)
            {
                return ret;
            }
            LOG_PRINT("auth password [%s]", packet->password);
        }
    }


    LOG_PRINT("In function packet_handler_conn");
    LOG_PRINT("Begin to check auth");
    if(packet->conn_f.f_uname)
    {
        if(check_auth(packet->username, packet->password) == 0)
        {
            LOG_PRINT("Check username %s, password %s success", packet->username, packet->password);
            return mqtt_conn_ack(packet, 0);
        }else{
            LOG_PRINT("Check username %s, password %s error", packet->username, packet->password);
            return mqtt_conn_ack(packet, 4);
        }
    }
}

int mqtt_conn_ack(struct mqtt_packet *packet, int ret_code)
{
    int ret;     
    struct mqtt_packet *ack_packet;
    ack_packet = malloc(sizeof(struct mqtt_packet));
    memset(ack_packet, '\0', sizeof(struct mqtt_packet));
    if(!ack_packet)
    {
        return MQTT_ERR_NOMEM;
    }
    ack_packet->fd = packet->fd;
    ack_packet->remain_length = 2;
    ack_packet->command = 0x20;
    ack_packet->dupflag = 0x00;
    ack_packet->qosflag = 0x00;
    ack_packet->retainflag = 0x00;
    ret = mqtt_packet_alloc(ack_packet);
    if(ret != MQTT_ERR_SUCCESS) return ret;
    ack_packet->payload[ack_packet->pos++] = 0x00;
    ack_packet->payload[ack_packet->pos++] = (uint8_t)ret_code;
    
    if((ret = mqtt_send_payload(ack_packet)) != MQTT_ERR_SUCCESS)
    {
        return  ret;
    }
    if(ret_code == 0)
    {
        mqtt_set_env(packet); // set client_data clientid
        mqtt_send_client_msg(packet->fd->sockfd, packet->identifier); 
    }
    return MQTT_ERR_SUCCESS;
}

int mqtt_send_client_msg(int sockfd, uint8_t *identifier)
{
    struct server_env *env = get_server_env();
    if(env == NULL)
    {
        return MQTT_ERR_NULL;
    }
    struct mqtt_string str_client_id;
    mqtt_string_alloc(&str_client_id, identifier, strlen(identifier));
    struct client_in_hash *client_node = (struct client_in_hash *) mqtt_hash_get(env->client_table, str_client_id);
    if(client_node == NULL)
    {
        LOG_PRINT("Can not find the node in the hash table");
        return -1;
    }
    if(client_node->mutex == 0) return -1; 
    client_node->mutex = 1;
    struct msg_node *n = client_node->head_nsend;
    while(n != NULL)
    {
        if(n->packet != NULL)
        {
            n->packet->remain_length = 0;
            n->packet->remain_length += strlen(n->packet->topic) + 2;
            if(n->packet->qosflag != 0)
            {
                n->packet->remain_length += 2;
            }
            n->packet->remain_length += strlen(n->packet->publish_content);
            mqtt_packet_alloc(n->packet);
            struct mqtt_packet *tmp_p = n->packet;
            int top_len = strlen(tmp_p->topic);
            if(top_len > 65535)
            {
                LOG_PRINT("Error: topic len is too long");
                continue;
            }
            uint8_t msb = (top_len & 0xf0) >> 8;
            uint8_t lsb = (top_len & 0x0f);
            tmp_p->payload[tmp_p->pos++] = msb;
            tmp_p->payload[tmp_p->pos++] = lsb;
            int i = 0;
            for(i = 0; i < top_len; i++)
            {
                tmp_p->payload[tmp_p->pos++] = tmp_p->topic[i];
            }
            if(tmp_p->qosflag != 0)
            {
                int id = mqtt_msg_id_gen();
                tmp_p->payload[tmp_p->pos++] = (id & 0xf0) >> 8;
                tmp_p->payload[tmp_p->pos++] = (id & 0x0f);
            }
            for(i = 0; i < strlen(tmp_p->publish_content); i++)
            {
                tmp_p->payload[tmp_p->pos++] = tmp_p->publish_content[i];
            }
            LOG_PRINT("Send a publish msg for client [%s]", tmp_p->identifier);
            tmp_p->fd->sockfd = sockfd;
            mqtt_read_payload(tmp_p);
        }else{
            LOG_PRINT("client [%s] has no msg_node", client_node->client_id.body);
        }
        n = n->next;
    }   
    client_node->head_nsend = client_node->tail_nsend = NULL; 
    client_node->mutex = 1;
    return 0;
}

void shut_dead_conn(int sockfd)
{
    struct server_env *env = get_server_env();
    assert(env != NULL);
    
    struct client_data *d;
    d = &env->clients[sockfd];
    remove_timer(env->timer_list, &(d->timer));
    removefd(env, sockfd);
}

int mqtt_handler_publish(struct mqtt_packet *packet)
{
    int ret, i;
    int sockfd = packet->fd->sockfd;
    uint8_t byte;

    if((ret = mqtt_parse_flags(packet)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    if((ret = mqtt_remain_length(packet)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }    

    if((ret = mqtt_read_payload(packet)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    
    if((ret = mqtt_str(packet, &(packet->msg.topic))) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    LOG_PRINT("Receive Publish Topic [%s]", packet->msg.topic); 
    if(packet->qosflag == 0x01 || packet->qosflag == 0x02)
    {
        if((ret = mqtt_payload_bytes(packet, packet->msg.id, 2)) != MQTT_ERR_SUCCESS)
        {
            return ret;
        }
        LOG_PRINT("Publish Message ID [%s]", packet->msg.id);
    }  

    mqtt_publish_content(packet);  
    LOG_PRINT("Publish Message Content [%s]", packet->msg.body);
    struct mqtt_topic *topic;
    struct mqtt_string *pstr_topic = mqtt_string_init(packet->msg.body);
    struct mqtt_string *pstr_topic_name  = mqtt_string_init(packet->msg.topic);
    topic = mqtt_topic_get(*pstr_topic);
    if(topic == NULL)
    {
        mqtt_topic_add(*pstr_topic_name, &topic);
    }
    assert(topic != NULL);
    _mqtt_topic_add_msg(topic, *pstr_topic);
    LOG_PRINT("Added the message [%s] to the topic [%s]", packet->msg.body, packet->msg.topic);    
    update_conn_timer(packet->fd->sockfd); 

    return MQTT_ERR_SUCCESS;
}
int update_conn_timer(int sockfd)
{
    struct server_env *env = get_server_env();
    assert(env);
    struct client_data *p_client = &(env->clients[sockfd]);
    return inc_timer(env->timer_list, &(p_client->timer)); 
}

int mqtt_handler_subscribe(struct mqtt_packet *packet)
{
    int ret, i;
    int sockfd = packet->fd->sockfd;
    uint8_t byte;
    struct server_env *env = get_server_env();
    if(!env)
    {
        return MQTT_ERR_NULL;
    } 
    
    if(!(env->clients[sockfd].client_id))
    {
        return MQTT_ERR_PROTOCOL;
    }
    
    if( (ret = mqtt_parse_flags(packet)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }    

    if( (ret = mqtt_remain_length(packet)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    if( (ret = mqtt_read_payload(packet)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    if(packet->qosflag == 0x01)
    {
        if( (ret = mqtt_payload_bytes(packet, packet->msg.id, 2))
            != MQTT_ERR_SUCCESS)
        {
            return ret;
        }
    }
    
    if( (ret = mqtt_parse_subtopices(packet)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    } 

    return MQTT_ERR_SUCCESS;    
}

int mqtt_set_env(struct mqtt_packet *packet)
{
    struct server_env *env = get_server_env();
    if(!env)
    {
        return MQTT_ERR_NULL;
    }
    
    struct client_data *client = &(env->clients[packet->fd->sockfd]);
    assert(packet->identifier != NULL);
    client->client_id = malloc(sizeof(uint8_t)* strlen(packet->identifier));
    assert(client->client_id);
    memcpy(client->client_id, packet->identifier, sizeof(uint8_t) * strlen(packet->identifier));
    struct mqtt_string *str_clientid = mqtt_string_init(packet->identifier);
    struct client_in_hash *c_node = (struct client_in_hash *)mqtt_hash_get(env->client_table, *str_clientid);
    if(c_node == NULL)
    {
        struct client_in_hash *c_node_tmp = malloc(sizeof(struct client_in_hash));
        assert(c_node_tmp != NULL);
        memset(c_node_tmp, '\0', sizeof(struct client_in_hash));
        c_node_tmp->sockfd = packet->fd->sockfd;
        mqtt_string_copy(str_clientid, &(c_node_tmp->client_id));
        c_node_tmp->head_nsend = NULL;
        c_node_tmp->tail_nsend = NULL;
        mqtt_hash_set(env->client_table, *str_clientid, (void *) c_node_tmp);  
        client->client_hash_node = c_node_tmp;
        LOG_PRINT("Create a client_in_hash node to the client table");
    }else{
        client->client_hash_node = c_node; 
        LOG_PRINT("Find the client_in_hash node in the client table");
    }
    
    return MQTT_ERR_SUCCESS;
}

int mqtt_handler_ping(struct mqtt_packet *packet)
{
    int ret;
    if( (ret = mqtt_remain_length(packet)) != MQTT_ERR_SUCCESS)
    {
       return ret; 
    }  
    struct server_env *env = get_server_env();
    if(!env)
    {
        return MQTT_ERR_NULL;
    }
    update_conn_timer(packet->fd->sockfd);
    mqtt_ping_resp(packet);
    return MQTT_ERR_SUCCESS;
}

int mqtt_ping_resp(struct mqtt_packet *packet)
{
    int ret;
    struct mqtt_packet *ack_packet;
    ack_packet = malloc(sizeof(struct mqtt_packet));
    memset(ack_packet, '\0', sizeof(struct mqtt_packet));
    if(!ack_packet)
    {
        return MQTT_ERR_NOMEM;
    } 
    ack_packet->fd = packet->fd;
    ack_packet->remain_length = 0;
    ack_packet->command = 0xd0;
    ret = mqtt_packet_alloc(ack_packet);
    mqtt_console_payload(ack_packet);
    if(ret != MQTT_ERR_SUCCESS) return ret;

    if( (ret == mqtt_send_payload(ack_packet)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    return MQTT_ERR_SUCCESS;
}

int mqtt_handler_disconnect(struct mqtt_packet *packet)
{
    int ret;
    if( (ret = mqtt_remain_length(packet)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }  
    
    return MQTT_ERR_SUCCESS;
}

int mqtt_handler_unsubscribe(struct mqtt_packet *packet)
{
    int ret;
    if( (ret = mqtt_parse_flags(packet)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    if( (ret = mqtt_remain_length(packet)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    if( packet->qosflag == 0x01 || packet->qosflag == 0x02)
    {
        if( (ret = mqtt_payload_bytes(packet, packet->msg.id, 2)) != MQTT_ERR_SUCCESS)
        {
            return ret;
        }
    }
    
    if( (ret = mqtt_parse_unsubtopices(packet)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    if( packet->qosflag == 0x01)
    {
        return mqtt_unsubscribe_ack(packet);
    }
    return MQTT_ERR_SUCCESS;
}

int mqtt_unsubscribe_ack(struct mqtt_packet *packet)
{
    assert(packet);
    struct mqtt_packet *ack_packet = malloc(sizeof(struct mqtt_packet));
    assert(ack_packet);
    memset(ack_packet, '\0', sizeof(struct mqtt_packet));
    ack_packet->fd = packet->fd;
    ack_packet->command = 0xb0;
    ack_packet->remain_length = 2;
    
    int ret = mqtt_packet_alloc(ack_packet);
    if( ret != MQTT_ERR_SUCCESS)
    {
        return ret;
    }

    ack_packet->payload[packet->pos++] = packet->msg.id[0];
    ack_packet->payload[packet->pos++] = packet->msg.id[1];

    if( (ret = mqtt_send_payload(ack_packet)) != MQTT_ERR_SUCCESS)
    {
        return ret; 
    }
    return MQTT_ERR_SUCCESS;
}
