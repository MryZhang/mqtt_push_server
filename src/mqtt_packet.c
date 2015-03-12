#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include "server.h"

int mqtt_packet_alloc(struct mqtt_packet *packet)
{
    uint32_t remain_length;
    uint8_t remain_bytes[5], byte, count = 0, i;

    remain_length = packet->remain_length;
    while(remain_length > 0 && count < 5)
    {
        byte = remain_length % 8;
        remain_length = remain_length >> 7;
        if(remain_length > 0)
        {
            byte = byte | 0x80;
        }
        remain_bytes[count++] = byte;
    }
    if(count == 5) return MQTT_ERR_PAYLOAD_SIZE;
    packet->remain_count = count;
    packet->packet_len = packet->remaining_length + 1 + packet->remain_count;
    packet->payload = (uint8_t *)malloc(sizeof(uint8_t)*packet->packet_len);
    if(!packet->payload) return MQTT_ERR_NOMEM;
    packet->payload[0] = packet->command;
    for(i = 0; i < count; i++)
    {
        packet->payload[i+1] = remain_bytes[i];
    }
    packet->pos = 1 + packet->remain_count;
    return MQTT_ERR_SUCCESS;
}
