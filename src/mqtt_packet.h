#ifndef _MQTT_PACKET_H_
#define _MQTT_PACKET_H_

#include <stdint.h>

struct mqtt_packet
{
    uint8_t *payload;
    uint8_t  command;
    /* the maxium remain length is 268435455 */
    uint32_t remain_length;
    /* the length of the remaining length bytes */
    uint8_t remain_count;
    uint32_t packet_len;
    uint32_t pos;
};

int mqtt_packet_alloc(struct mqtt_packet *packet);

#endif
