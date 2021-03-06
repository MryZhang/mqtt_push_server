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
    if(packet->alive_timer <= 0)
    {
        packet->alive_timer = 30;
    }

    struct server_env *env = get_server_env();
    assert(env != NULL);
    struct client_data *client = &(env->clients[packet->fd->sockfd]);
    time_t now = time(NULL);    
    client->expire_time = packet->alive_timer + packet->alive_timer / 2;
    // add the clean flag to the client _data
    client->f_clean = packet->conn_f.f_clean;
    // caculate the expire time 
    client->timer.expire = now + client->expire_time;
    assert(add_timer(env->timer_list, &(client->timer))== MQTT_ERR_SUCCESS);
    if((ret = mqtt_str(packet, &packet->identifier)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }  
    LOG_PRINT("client identitfier [%s]", packet->identifier);
    int len = 0;
    while(packet->identifier[len] != '\0') len++;
    // mqtt prototcal v3.1 : the length of identifier is less than 23
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
        LOG_PRINT("ClientID [%s] is being used by another", packet->identifier);
        return MQTT_ERR_ID_REJECTED;
    }


    if(packet->conn_f.f_will)
    {
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
    LOG_PRINT("send a ret_code [%d] to client [%s]", ret_code, packet->identifier);
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
        mqtt_send_client_msg(packet->fd->sockfd); 
    }
    return MQTT_ERR_SUCCESS;
}

int mqtt_send_client_msg(int sockfd)
{
    LOG_PRINT("send messages which in client_in_hash");
    struct server_env *env = get_server_env();
    if(env == NULL)
    {
        return MQTT_ERR_NULL;
    }
    struct client_data *client = &(env->clients[sockfd]);
    struct client_in_hash *client_node = client->client_hash_node;
    if(client_node == NULL)
    {
        LOG_PRINT("Error: client_hash_node is not init");
        return -1;
    }
    
    struct msg_node *n = client_node->head_nsend;
    LOG_PRINT("AAA");
    while(n != NULL)
    {
        LOG_PRINT("START");
        if(n->packet != NULL)
        {
             
            LOG_PRINT("AAA");
            n->packet->remain_length = 0;
            n->packet->remain_length += strlen(n->packet->topic) + 2;
            if(n->packet->qosflag != 0)
            {
                n->packet->remain_length += 2;
            }
            n->packet->remain_length += strlen(n->packet->publish_content);
            LOG_PRINT("Remain_length [%d]", n->packet->remain_length);
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
            
            LOG_PRINT("Format the publish content");
            assert(tmp_p->publish_content != NULL);
            LOG_PRINT("Publish_Content Len [%d], content [%s]", strlen(tmp_p->publish_content), tmp_p->publish_content);
            LOG_PRINT("Payload Len [%d]", tmp_p->packet_len);
            int publish_len = strlen(tmp_p->publish_content);
            for(i = 0; i < publish_len; i++)
            {
                LOG_PRINT("Pos [%d]", tmp_p->pos);
                tmp_p->payload[tmp_p->pos++] = tmp_p->publish_content[i];
            }
            tmp_p->fd = malloc(sizeof(struct fds));
            assert(tmp_p->fd != NULL);
            tmp_p->fd->sockfd = sockfd;
            mqtt_send_payload(tmp_p);
        }else{
            LOG_PRINT("client [%s] has no msg_node", client_node->client_id.body);
        }
        n = n->next;
    }   
    client_node->head_nsend = client_node->tail_nsend = NULL; 
    return 0;
}

void shut_dead_conn(int sockfd)
{
    printf("The client [%d] shut down\n", sockfd);
    LOG_PRINT("Need to shut conn sockfd [%d]", sockfd);
    struct server_env *env = get_server_env();
    assert(env != NULL);
    
    struct client_data *d;
    d = &env->clients[sockfd];
    if(d->client_id == NULL)
    {   
        LOG_PRINT("In function shut_dead_conn");
        LOG_PRINT("the client has no id");
    }else{
        rm_client_id(d->client_id);
        // if clean session is set then remove everything about this client
        if(d->f_clean == 0x01)
        {
            rm_client_node(d->client_id);
        }
        LOG_PRINT("Info Remove the Clientid [%s] from Redis", d->client_id);
    }
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
    LOG_PRINT("Publish Topic Content [%s]", packet->msg.topic);
    struct mqtt_topic *topic;
    struct mqtt_string *pstr_topic = malloc(sizeof(struct mqtt_string));
    mqtt_string_alloc(pstr_topic, packet->msg.body, strlen(packet->msg.body));
    struct mqtt_string *pstr_topic_name  = malloc(sizeof(struct mqtt_string));
    mqtt_string_alloc(pstr_topic_name,packet->msg.topic, strlen(packet->msg.topic));
    LOG_PRINT("X");
    topic = mqtt_topic_get(*pstr_topic_name);
    LOG_PRINT("Y");
    if(topic == NULL)
    {
        LOG_PRINT("Z");
        mqtt_topic_add(*pstr_topic_name, &topic);
    }
    LOG_PRINT("A");
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
    return inc_timer(env->timer_list, &(p_client->timer), p_client->expire_time); 
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
    LOG_PRINT("In function mqtt_set_env");
    struct server_env *env = get_server_env();
    if(!env)
    {
        return MQTT_ERR_NULL;
    }
    
    struct client_data *client = &(env->clients[packet->fd->sockfd]);
    assert(packet->identifier != NULL);
    int len = strlen(packet->identifier) + 1;
    client->client_id = malloc(sizeof(uint8_t)* len);
    assert(client->client_id);
    memset(client->client_id, '\0', len);
    memcpy(client->client_id, packet->identifier, sizeof(uint8_t) * strlen(packet->identifier));
    struct mqtt_string *str_clientid = malloc(sizeof(struct mqtt_string));
    mqtt_string_alloc(str_clientid, packet->identifier, strlen(packet->identifier));
    LOG_PRINT("A"); 
    struct client_in_hash *c_node = (struct client_in_hash *)mqtt_hash_get(env->client_table, *str_clientid);
    LOG_PRINT("B");
    if(c_node == NULL)
    {
        LOG_PRINT("Can not find the client");
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
    LOG_PRINT("Send a ping resp to the client");
    int ret;
    struct mqtt_packet *ack_packet;
    ack_packet = malloc(sizeof(struct mqtt_packet));
    if(!ack_packet)
    {
        return MQTT_ERR_NOMEM;
    } 
    memset(ack_packet, '\0', sizeof(struct mqtt_packet));
    ack_packet->fd = packet->fd;
    ack_packet->remain_length = 0;
    ack_packet->command = 0xd0;
    ret = mqtt_packet_alloc(ack_packet);
    if(ret != MQTT_ERR_SUCCESS) return ret;

    LOG_PRINT("Start to send ping resp"); 
    if( (ret == mqtt_send_payload(ack_packet)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    return MQTT_ERR_SUCCESS;
}

int mqtt_handler_disconnect(struct mqtt_packet *packet)
{
    int ret;
    LOG_PRINT("handler disconnect");
    if( (ret = mqtt_remain_length(packet)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }  
    LOG_PRINT("Info: handler disconnect"); 
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
