#include "mqtt_handler.h"
#include "mqtt_packet.h"
#include "server.h"

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
    
    
    return MQTT_ERR_SUCCESS;  
}
