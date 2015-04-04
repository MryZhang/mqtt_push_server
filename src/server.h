#ifndef _SERVER_H_
#define _SERVER_H_

#include "hiredis.h"
#include "util.h"

/* Error Values */
enum mqtt_msg_t {
    MQTT_ERR_SUCCESS = 0,
    MQTT_ERR_NOMEM = 1, /* malloc failure */
    MQTT_ERR_PROTOCOL = 2, /* protocal failure */
    MQTT_ERR_PAYLOAD_SIZE = 3,
    MQTT_ERR_ID_TOO_LONG = 4,
    MQTT_ERR_REDIS = 5,
    MQTT_ERR_ID_REJECTED = 6,
    MQTT_ERR_NULL = 7,
};
/* Get the context of the redis database */
redisContext *getRedisContext();
struct server_env *get_server_env();

#define TIMESLOT 1

#endif
