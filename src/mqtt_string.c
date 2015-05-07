#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "mqtt_string.h"

void mqtt_string_alloc(struct mqtt_string *string, uint8_t *str)
{
    assert(string != NULL);
    string->len = strlen(str);
    string->body = malloc(string->len * sizeof(uint8_t));
    assert(string->body);
    memcpy(string->body, str, string->len);
}

int mqtt_string_copy(struct mqtt_string *src, struct mqtt_string *dst)
{
    int i;
    if(!src || !dst) return -1;
    dst->len = src->len;
    dst->body = malloc(sizeof(uint8_t) * dst->len);
    assert(dst->body);
    for(i = 0; i < src->len; i++)
    {
        dst->body[i] = src->body[i];
    }
    return 0;
}

int mqtt_string_cmp(struct mqtt_string str_foo, struct mqtt_string str_bar)
{
    if( str_foo.len != str_bar.len)  return -1;
    int i;
    for(i = 0; i < str_bar.len; i++)
    {
        if(str_foo.body[i] != str_bar.body[i])
        {
            return -1;
        }
    }
    return 0;
}

struct mqtt_string *mqtt_string_init(uint8_t *str)
{
    struct mqtt_string *string;
    int len = strlen(str);
    string = malloc(sizeof(uint8_t) * len);
    assert(string);
    memcpy(string->body, str, len);
    return string;
}
