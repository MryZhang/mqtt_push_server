#ifndef _MQTT_STRING_H_
#define _MQTT_STRING_H_
#include <stdint.h>

struct mqtt_string
{
    uint32_t len;
    uint8_t *body;
};

void mqtt_string_alloc(struct mqtt_string *string, uint8_t *str); 
int mqtt_string_copy(struct mqtt_string *src, struct mqtt_string *dst);
int mqtt_string_cmp(struct mqtt_string str_foo, struct mqtt_string str_bar);
struct mqtt_string *mqtt_string_init(uint8_t *str);

#endif
