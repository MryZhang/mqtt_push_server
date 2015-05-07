#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "mqtt_handler.h"
#include "mqtt_packet.h"
#include "server.h"
#include "hiredis.h"
#include "client_ds.h"
#include "net.h"
#include "redis_com.h"
#include "mqtt_message.h"

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

    if((ret = mqtt_read_livetimer(packet)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    printf("func conn_handler: begin to read id\n");
    if((ret = mqtt_str(packet, &packet->identifier)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }  
    int len = 0;
    while(packet->identifier[len] != '\0') len++;
    if(len > 23)
    {
        mqtt_conn_ack(packet, 2);
        return MQTT_ERR_ID_TOO_LONG;
    }

    printf("packet->identifier : %s\n", packet->identifier);
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
        if((ret = mqtt_str(packet, &packet->will_topic)) != MQTT_ERR_SUCCESS)
        {
            return ret;
        } 
        if((ret = mqtt_str(packet, &packet->will_message)) != MQTT_ERR_SUCCESS)
        {
            return ret;
        }
    }
    if(packet->conn_f.f_uname)
    {
        if((ret = mqtt_str(packet, &packet->username)) != MQTT_ERR_SUCCESS)
        {
            return ret;
        }
        
        if(packet->conn_f.f_pwd)
        {
            if((ret = mqtt_str(packet, &packet->password)) != MQTT_ERR_SUCCESS)
            {
                return ret;
            }
        }
    }

    return mqtt_conn_ack(packet, 0);  
}

int mqtt_conn_ack(struct mqtt_packet *packet, int ret_code)
{
    printf("Begin to send conn_ack============\n");
    int ret;     
    struct mqtt_packet *ack_packet;
    ack_packet = malloc(sizeof(struct mqtt_packet));
    if(!ack_packet)
    {
        printf("func mqtt_conn_ack: malloc failure\n");
        return MQTT_ERR_NOMEM;
    }
    ack_packet->fd = packet->fd;
    ack_packet->remain_length = 2;
    printf("remain length in conn ack: %d\n", ack_packet->remain_length);
    ack_packet->command = 0x20;
    ack_packet->dupflag = 0x00;
    ack_packet->qosflag = 0x00;
    ack_packet->retainflag = 0x00;
    ret = mqtt_packet_alloc(ack_packet);
    if(ret != MQTT_ERR_SUCCESS) return ret;
    ack_packet->payload[packet->pos++] = 0x00;
    ack_packet->payload[packet->pos++] = (uint8_t)ret_code;
    
    mqtt_console_payload(ack_packet); 
    if((ret = mqtt_send_payload(ack_packet)) != MQTT_ERR_SUCCESS)
    {
        return  ret;
    }
    return MQTT_ERR_SUCCESS;
}

void shut_dead_conn(int sockfd)
{
    printf("Shut a dead conn: %d\n", sockfd);
    struct server_env *env = get_server_env();
    assert(env != NULL);
    
    struct client_data *d;
    d = &env->clients[sockfd];
    remove_timer(env->timer_list, &(d->timer));
    removefd(env, sockfd);
    printf("shut_dead_conn\n");    
}

int mqtt_handler_publish(struct mqtt_packet *packet)
{
    int ret, i;
    int sockfd = packet->fd->sockfd;
    uint8_t byte;

    printf("Info: begin to handler publish.\n");
    if((ret = mqtt_parse_flags(packet)) != MQTT_ERR_SUCCESS)
    {
        printf("Error:  parse flags.\n");
        return ret;
    }
    if((ret = mqtt_remain_length(packet)) != MQTT_ERR_SUCCESS)
    {
        printf("Error:  remain_length.\n");
        return ret;
    }    

    if((ret = mqtt_read_payload(packet)) != MQTT_ERR_SUCCESS)
    {
        printf("Error:  read payload.\n");
        return ret;
    }
    
    if((ret = mqtt_str(packet, &(packet->msg.topic))) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    
    if(packet->qosflag == 0x01 || packet->qosflag == 0x02)
    {
        if((ret = mqtt_str(packet, &(packet->msg.id))) != MQTT_ERR_SUCCESS)
        {
            return ret;
        }
    }  

    if((ret = mqtt_str(packet, &(packet->msg.body))) != MQTT_ERR_SUCCESS)
    {
        return ret;
    } 
    
    struct mqtt_topic *topic;
    struct mqtt_string *pstr_topic = mqtt_string_init(packet->msg.body);
    topic = mqtt_topic_get(*pstr_topic);
    if(topic == NULL)
    {
        mqtt_topic_add(*pstr_topic, NULL);
    }
    _mqtt_topic_add_msg(topic, *pstr_topic);
    //free(packet->payload);
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
