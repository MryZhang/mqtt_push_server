#include <stdlib.h>
#include <string.h>

#include "mqtt_handler.h"
#include "mqtt_packet.h"
#include "server.h"
#include "hiredis.h"

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
    printf("The Payload: \n");
    for(i = 0; i < packet->remain_length; i++)
    {
        printf("%x  ", packet->payload[i]);
    }
    printf("\n");
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

    return MQTT_ERR_SUCCESS;  
}

int mqtt_conn_ack(struct mqtt_packet *packet, int ret_code)
{
    int ret;     
    struct mqtt_packet *ack_packet;
    ack_packet = malloc(sizeof(struct mqtt_packet));
    ack_packet->fd = packet->fd;
    ack_packet->remain_length = 2;
    ack_packet->command = 0x20;
    ack_packet->dupflag = 0x00;
    ack_packet->qosflag = 0x00;
    ack_packet->retainflag = 0x00;
    ret = mqtt_packet_alloc(packet);
    if(ret != MQTT_ERR_SUCCESS) return ret;
    ack_packet->payload[packet->pos++] = 0x00;
    ack_packet->payload[packet->pos++] = (uint8_t)ret_code;
    
    return MQTT_ERR_SUCCESS;
}
