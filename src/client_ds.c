#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "client_ds.h"
#include "util.h"
#include "server.h" 
#include "mqtt_hash.h"

static struct mqtt_hash_t *client_table;

struct mqtt_hash_t *get_client_table()
{
    if(!client_table)
    {
        struct server_env *env = get_server_env();
        if(!env) return NULL;
        return env->client_table;
    }
    return client_table;
    
}
struct mqtt_client_in_hash *mqtt_get_client_s(struct mqtt_string client_id)
{
    struct mqtt_hash_t *hash_t = get_client_table();
    if(!hash_t) return NULL;
    return _mqtt_get_client_s(hash_t, client_id); 
}
struct mqtt_client_in_hash *_mqtt_get_client_s(struct mqtt_hash_t* hash_t, struct mqtt_string client_id)
{
    struct mqtt_hash_n *hash_n = mqtt_hash_get(hash_t, client_id);
    if(hash_n != NULL)
    {
       struct mqtt_client_in_hash *client_in_hash = (struct mqtt_client_in_hash *)(hash_n->data.body);
       return client_in_hash;
    } 
    return NULL;
}
int mqtt_add_client_s(struct mqtt_string client_id, int sockfd)
{
    struct mqtt_hash_t *hash_t = get_client_table();
    if(!hash_t) return -1;
    return _mqtt_add_client_s(hash_t, client_id, sockfd);
}
int _mqtt_add_client_s(struct mqtt_hash_t *hash_t, struct mqtt_string client_id, int sockfd)
{
    struct client_in_hash *client_n = _mqtt_init_client_in_hash(client_id, sockfd);
    if(!client_n) return -1;
    struct mqtt_string *str = mqtt_string_init((uint8_t *) client_n);
    if(!str) return -1; 
    return mqtt_hash_set(hash_t, client_id, *str);
}

struct client_in_hash *_mqtt_init_client_in_hash(struct mqtt_string client_id, int sockfd)
{
    struct client_in_hash *node = malloc(sizeof(struct client_in_hash));
    assert(node);
    memset(node, '\0', sizeof(struct client_in_hash));
    node->sockfd = sockfd;
    mqtt_string_copy(&(node->client_id), &client_id);
    node->head_nsend = node->tail_nsend = NULL;
    return node;
}
