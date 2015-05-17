#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "mqtt_string.h"
#include "log.h"

void mqtt_string_alloc(struct mqtt_string *string, uint8_t *str, int len)
{
    assert(string != NULL);
    //string->len = strlen(str);  data struct strlen wrong 
    assert(len > 0);
    string->len = len;
    string->body = malloc((len + 1) * sizeof(uint8_t));
    assert(string->body);
    memset(string->body, '\0', (len + 1));
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
    dst->body[i] = '\0';
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

