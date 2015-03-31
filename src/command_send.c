#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "server.h"
#include "mqtt3_protocal.h"
#include "mqtt_packet.h"

int mqtt_send_connack(int sockfd, uint8_t ret)
{
    struct mqtt_packet *packet;
    int rc;

    packet = (struct mqtt_packet *)malloc(sizeof(struct mqtt_packet));
    if(!packet) return MQTT_ERR_NOMEM;
    packet->remain_length = 2;
    packet->command = CONNACK;
    
    rc = mqtt_packet_alloc(packet);
    if(rc)
    {
        free(packet);
        return rc;
    }
    
    packet->payload[++packet->pos] = ret;
        
    return MQTT_ERR_SUCCESS;
}
