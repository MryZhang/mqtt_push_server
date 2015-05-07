#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "server.h"
#include "mqtt_packet.h"
#include "mqtt_message.h"
#include "net.h"

/* build packet */
int mqtt_packet_alloc(struct mqtt_packet *packet)
{
    uint32_t remain_length;
    uint8_t remain_bytes[5], byte, count = 0, i;

    printf("remain length %d at begin of alloc.\n", packet->remain_length);
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
    printf("func packet alloc:============\n");
    printf("count: %d\n", count);
    printf("remain length : %d\n", packet->remain_length);
    packet->packet_len = packet->remain_length + 1 + packet->remain_count;
    packet->payload = (uint8_t *)malloc(sizeof(uint8_t)*packet->packet_len);
    if(!packet->payload) return MQTT_ERR_NOMEM;
    packet->payload[0] = mqtt_fix_header(packet);
    for(i = 0; i < count; i++)
    {
        packet->payload[i+1] = remain_bytes[i];
    }
    packet->pos = 1 + packet->remain_count;
    return MQTT_ERR_SUCCESS;
}
uint8_t mqtt_fix_header(struct mqtt_packet *packet)
{
    uint8_t byte = (packet->command & 0xf0) |  (packet->dupflag & 0x01 << 3) | (packet->qosflag & 0x03 << 1) | (packet->retainflag & 0x01);
   printf("func: fix_head byte: 0x%x\n", byte); 
    return byte;
}
/* calculate the remain_length */
int mqtt_remain_length(struct mqtt_packet *packet)
{
    int multiplier = 1, recv_len, byte_num = 0;
    uint32_t value = 0;
    uint8_t digit;
    do{
        recv_len = mqtt_net_read(packet->fd->sockfd, (void *)&digit, 1);
        printf("digit is %d\n", digit);
        if(recv_len == 1)
        {
            value += (digit & 0x7F) * multiplier;
            multiplier *= 128;
            byte_num++;
        }else{
            return MQTT_ERR_PROTOCOL;
        }
    }while((digit & 0x80) != 0 && byte_num <= 4);
    packet->remain_length = value;
    return MQTT_ERR_SUCCESS;
}
int mqtt_read_payload(struct mqtt_packet *packet)
{
    int recv_len = 0, ret = 0, remain_length;
    remain_length = packet->remain_length;
    packet->payload = malloc(sizeof(uint8_t)*remain_length);
    assert(packet->payload != NULL);
    while((ret = mqtt_net_read(packet->fd->sockfd, &(packet->payload[recv_len]), remain_length)) > 0)
    {
        recv_len += ret;
        remain_length -= ret;
    }
    assert(remain_length == 0);
    if(remain_length > 0)
    {
        return MQTT_ERR_PROTOCOL;
    }
    return MQTT_ERR_SUCCESS;
}
int mqtt_payload_byte(struct mqtt_packet *packet, uint8_t *byte)
{
    if(packet->pos < packet->remain_length)
    {
        *byte = packet->payload[packet->pos];
        if(isalpha(*byte) || isdigit(*byte))
        {
            printf("func payload_byte: had read %c\n", *byte);
        }else{
            printf("func payload_byte: had read 0x%x\n", *byte);
        }
        packet->pos++;
        return MQTT_ERR_SUCCESS;
    }else{
        return MQTT_ERR_PROTOCOL;
    }
}
int mqtt_payload_bytes(struct mqtt_packet *packet, uint8_t *bytes, uint16_t len)
{
    int ret;
    uint16_t read_len = 0;
    assert(bytes);
    while(read_len < len && (ret = mqtt_payload_byte(packet, &bytes[read_len])) == MQTT_ERR_SUCCESS)
    {
        read_len++;
    }
    printf("Info:  read_len [%d].\n", read_len);
    if(read_len < len)
    {
        return MQTT_ERR_PROTOCOL;
    }else{
        return MQTT_ERR_SUCCESS;
    }
}
int mqtt_str(struct mqtt_packet *packet, uint8_t **pstr)
{
    int ret;
    uint16_t str_len = 0;
    uint8_t len_msb, len_lsb;
    if((ret = mqtt_payload_byte(packet, &len_msb)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    
    if((ret = mqtt_payload_byte(packet, &len_lsb)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    str_len = len_msb*16 + len_lsb;
    printf("func mqtt_str: len_msb: %d\n", len_msb);
    printf("func mqtt_str: len_lsb: %d\n", len_lsb); 
    printf("func mqtt_str: str_len:%d\n", str_len);
    (*pstr) = malloc(sizeof(uint8_t) * (str_len+1));
    (*pstr)[str_len] = '\0';
    if((ret = mqtt_payload_bytes(packet, *pstr, str_len)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
}
int mqtt_read_protocol_name(struct mqtt_packet *packet)
{
    uint8_t *chptr;
    int ret;
    if((ret = mqtt_str(packet, &chptr) ) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    if(strncmp(chptr, "MQTT", 4) == 0)
    {
        printf("please use v3.1\n");
        return MQTT_ERR_PROTOCOL;
    }
    if(strncmp(chptr, "MQIsdp", 6) != 0)
    {
        return MQTT_ERR_PROTOCOL;
    }
    return MQTT_ERR_SUCCESS;
}
int mqtt_read_protocol_version(struct mqtt_packet *packet)
{
    int ret;
    uint8_t byte;
    if((ret = mqtt_payload_byte(packet, &byte)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    if(byte != 3)
    {
        return MQTT_ERR_PROTOCOL;
    }
    return MQTT_ERR_SUCCESS;
}
int mqtt_read_connect_flags(struct mqtt_packet *packet)
{
    int ret;
    uint8_t byte;
    if((ret = mqtt_payload_byte(packet, &byte)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    
    packet->conn_f.f_uname = byte & 0x80 >> 7;
    packet->conn_f.f_pwd = byte & 0x40 >> 6;
    packet->conn_f.f_will_retain = byte & 0x20 >> 5;
    packet->conn_f.f_will_qos = byte & 0x18 >> 3;
    packet->conn_f.f_will = byte & 0x04 >> 2;
    packet->conn_f.f_clean = byte & 0x02 >> 1;
    return MQTT_ERR_SUCCESS;
}

int mqtt_read_livetimer(struct mqtt_packet *packet)
{
    int ret;
    uint8_t timer_msb;
    uint8_t timer_lsb;
    uint16_t timer;
    if((ret = mqtt_payload_byte(packet, &timer_msb)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }

    if((ret = mqtt_payload_byte(packet, &timer_lsb)) != MQTT_ERR_SUCCESS)
    {
        return ret;
    }
    
    timer = timer_msb * 16 + timer_lsb;
    packet->alive_timer = timer;

    return MQTT_ERR_SUCCESS;
}

int mqtt_send_payload(struct mqtt_packet *packet)
{
    printf("Begin to send payload==========\n");
    struct server_env *env = get_server_env();
    assert(env);
    set_fd_out(env, packet->fd->sockfd, (void *)packet->payload, packet->packet_len);
    printf("End of send payload============\n");
    return MQTT_ERR_SUCCESS;
}

void mqtt_console_payload(struct mqtt_packet *packet)
{
    printf("Print payload=============\n");
    printf("payload len :%d \n", packet->packet_len);
    int i;
    for(i = 0; i < packet->packet_len; i++)
    {
        printf("0x%x ", packet->payload[i]);
    }
    printf("end of print==============\n");
}

int mqtt_parse_flags(struct mqtt_packet *packet)
{
    packet->dupflag = (packet->command & 0x08) >> 3;
    packet->qosflag = (packet->command & 0x06) >> 1;
    packet->retainflag = (packet->command & 0x01);
    if(packet->qosflag != 0x00 && packet->qosflag != 0x01 && packet->qosflag != 0x02) return MQTT_ERR_PROTOCOL;
    return MQTT_ERR_SUCCESS;
}
int mqtt_publish_content(struct mqtt_packet *packet)
{
    int i, lcontent, j;
    assert(packet->pos < packet->remain_length);
    lcontent = packet->remain_length - packet->pos + 1;
    printf("Info: lcontent [%d]\n", lcontent);
    packet->msg.body = malloc(sizeof(uint8_t) * lcontent);
    if(!packet->msg.body)
    {
        printf("Err: publish_content mem failure.\n");
        return MQTT_ERR_NOMEM;
    }
    for(i = packet->pos, j = 0; i < packet->remain_length; i++, j++)
    {
        printf("Info: i [%d], j [%d]\n", i, j);
        packet->msg.body[j] = packet->payload[i];
    }
    packet->msg.body[j] = '\0';
    return MQTT_ERR_SUCCESS;
}

void mqtt_packet_format(struct mqtt_packet *packet)
{
    printf("Packet Detail: \n");
    printf("    connfd: %d,\n", packet->fd->sockfd);
    printf("    dupflag: %d,\n", packet->dupflag);
    printf("    qosflag: %d,\n", packet->qosflag);
    printf("    remain_length: %d,\n", packet->remain_length);
    printf("    remain_count: %d,\n", packet->remain_count);
    printf("    packet_len: %d,\n", packet->packet_len);
    printf("    pos: %d,\n", packet->pos);
    printf("End of Packet Detai\n\n");
}

int mqtt_parse_subtopices(struct mqtt_packet *packet)
{
    while(packet->pos < packet->remain_length)
    {
        uint8_t *p_topic;
        mqtt_str(packet, &p_topic); 
        uint8_t qos;
        mqtt_payload_byte(packet, &qos);
        printf("Info: topic name [%s]\n", p_topic);
        printf("Info: topic qos [%d]\n", qos);
        struct mqtt_string ms_topic;
        mqtt_string_alloc(&ms_topic, p_topic, strlen(p_topic));

        struct mqtt_topic *topic = mqtt_topic_get(ms_topic);
        if(!topic)
        {
            printf("Info: topic [%s] is not existed!\n");    
        }else{
            printf("Info: topic [%s] is existed.\n"); 
        }
    }    
    return MQTT_ERR_SUCCESS;
}
