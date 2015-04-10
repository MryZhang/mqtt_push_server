#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mqtt_string.h"
#include "mqtt_hash.h"


int mqtt_hash_init(struct mqtt_hash_t **hash_t)
{
    *hash_t = malloc(sizeof(struct mqtt_hash_t));
    assert(*hash_t);
    memset(*hash_t, '\0', sizeof(struct mqtt_hash_t));
    int i = 0;
    for(i = 0; i < HASH_LEN; i++)
    {
        (*hash_t)->iterms[i] = NULL;
    }
    return 0;
}

int mqtt_hash_set(struct mqtt_hash_t *hash_t, struct mqtt_string key, struct mqtt_string data)
{
    int i, index = 0;
    index = _mqtt_hash_calinx(key);
    struct mqtt_hash_n *hiterm;
    if(hash_t->iterms[index] == NULL)
    {
        struct mqtt_hash_n *node = malloc(sizeof(struct mqtt_hash_n));
        assert(node);

        hash_t->iterms[index] = node;
        mqtt_string_copy(&key, &(node->key));
        mqtt_string_copy(&data, &(node->data));
        return 0;
    }
    struct mqtt_hash_n *tmp;
    tmp = hash_t->iterms[index];
    while(tmp->next != NULL && mqtt_string_cmp(tmp->key, key) != 0)
    {
        tmp = tmp->next;
    }

    if(mqtt_string_cmp(tmp->key, key) == 0)
    {
        mqtt_string_copy(&key, &(tmp->key));
    }else{
        struct mqtt_hash_n *node = malloc(sizeof(struct mqtt_hash_n));
        assert(node);

        mqtt_string_copy(&key, &(node->key));
        mqtt_string_copy(&data, &(node->data));
        tmp->next = node;
        node->next = NULL;
    }
    return 0;
}

struct mqtt_hash_n * mqtt_hash_del(struct mqtt_hash_t *hash_t, struct mqtt_string key)
{
    int index;
    index = _mqtt_hash_calinx(key);
    struct mqtt_hash_n *tmp = hash_t->iterms[index];
    if(tmp == NULL)
    {
        return NULL;
    }
    if(mqtt_string_cmp(tmp->key, key) == 0)
    {
        struct mqtt_hash_n *ret;
        ret = tmp;
        hash_t->iterms[index] = tmp->next;
        return ret;
    }
    while(tmp->next != NULL && mqtt_string_cmp(tmp->next->key, key))
    {
        tmp = tmp->next;
    }

    if(tmp->next == NULL)
    {
        return NULL;
    }else{
        struct mqtt_hash_n *ret;
        ret = tmp->next;
        tmp->next = ret->next;
        return ret;
    }
}

int _mqtt_hash_calinx(struct mqtt_string key)
{
    int index, i;
    for(i = 0; i < key.len; i++)
    {
        index += key.body[i];
    }
    index /= 26;
    return index;
}

struct mqtt_hash_n *mqtt_hash_get(struct mqtt_hash_t *hash_t, struct mqtt_string key)
{
    if(!hash_t)
    {
        printf("func mqtt_hash_n hash_t : NULL\n");
        return NULL;
    }
    int index = _mqtt_hash_calinx(key);
    struct mqtt_hash_n *node = hash_t->iterms[index];
    while(node && mqtt_string_cmp(node->key, key) == -1)
    {
        node = node->next;
    }
    return node;
}
