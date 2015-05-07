#ifndef _MQTT_PACKET_H_
#define _MQTT_PACKET_H_

#include <stdint.h>
#include "net.h"

struct conn_flag
{
    uint8_t f_uname;
    uint8_t f_pwd;
    uint8_t f_will_retain;
    uint8_t f_will_qos;
    uint8_t f_will;
    uint8_t f_clean;
};
struct mqtt_message
{
    uint8_t id[2];
    uint8_t *topic;
    uint8_t *body;
};
struct mqtt_packet
{
    struct fds *fd;
    uint8_t *payload;
    uint8_t  command;
    uint8_t dupflag;
    uint8_t qosflag;
    uint8_t retainflag;
    /* the maxium remain length is 268435455 */
    uint32_t remain_length;
    /* the length of the remaining length bytes */
    uint8_t remain_count;
    uint32_t packet_len;
    uint32_t pos;
    struct conn_flag conn_f;
    uint16_t alive_timer;
    uint8_t *identifier;
    uint8_t *will_topic;
    uint8_t *will_message;
    uint8_t *username;
    uint8_t *password;
    struct mqtt_message msg;
    uint8_t *publish_content;
};

int mqtt_packet_alloc(struct mqtt_packet *packet);
int mqtt_remain_length(struct mqtt_packet *packet);
int mqtt_read_payload(struct mqtt_packet *packet);
int mqtt_payload_byte(struct mqtt_packet *packet, uint8_t *byte);
int mqtt_payload_bytes(struct mqtt_packet *packet, uint8_t *bytes, uint16_t len);
int mqtt_str(struct mqtt_packet *packet, uint8_t **pstr);
int mqtt_read_protocol_name(struct mqtt_packet *packet);
int mqtt_read_protocol_version(struct mqtt_packet *packet);
int mqtt_read_connect_flags(struct mqtt_packet *packet);
int mqtt_read_livetimer(struct mqtt_packet *packet);

uint8_t mqtt_fix_header(struct mqtt_packet *packet);
int mqtt_send_payload(struct mqtt_packet *packet);
void mqtt_console_payload(struct mqtt_packet *packet);
int mqtt_parse_flags(struct mqtt_packet *packet);
void mqtt_packet_format(struct mqtt_packet *packet);
int mqtt_publish_content(struct mqtt_packet *packet);
#endif
