#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "mqtt_string.h"

void mqtt_string_alloc(struct mqtt_string *string, uint8_t *str, int len)
{
    assert(string != NULL);
    //string->len = strlen(str);  data struct strlen wrong 
    assert(len > 0);
    string->len = len;
    //printf("Info: alloc len [%d]\n", string->len);
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
    /*
    printf("Info: dst len [%d]\n", dst->len);
    printf("Info: src body [%s]\n", src->body);
    printf("Info: dst body [%s]\n", dst->body);
    */
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

struct mqtt_string *mqtt_string_init(const uint8_t *str)
{
    struct mqtt_string *string;
    int len = strlen(str);
    string = malloc(sizeof(struct mqtt_string));
    assert(string);
    string->body = malloc(sizeof(uint8_t) * len);
    assert(string->body);
    
    memcpy(string->body, str, len);
    string->len = len;
    return string;
}
