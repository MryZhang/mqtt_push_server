#ifndef _MQTT_HASH_H_
#define _MQTT_HASH_H_

#include "mqtt_string.h"
#define HASH_LEN 26
struct mqtt_hash_t
{
    struct mqtt_hash_n *iterms[HASH_LEN];       
};

struct mqtt_hash_n
{
    struct mqtt_string key;
    void * data; 
    struct mqtt_hash_n *next;
};
int mqtt_hash_init(struct mqtt_hash_t **hash_t);
int mqtt_hash_set(struct mqtt_hash_t *hash_t, struct mqtt_string key, void *data);
void *mqtt_hash_del(struct mqtt_hash_t *hash_t, struct mqtt_string key);
int _mqtt_hash_calinx(struct mqtt_string key);
void *mqtt_hash_get(struct mqtt_hash_t *hash_t, struct mqtt_string key);
#endif
