#ifndef _SERVER_H_
#define _SERVER_H_

/* Error Values */
enum mqtt_msg_t {
    MQTT_ERR_SUCCESS = 0,
    MQTT_ERR_NOMEM = 1, /* malloc failure */
    MQTT_ERR_PROTOCOL = 2, /* protocal failure */
    MQTT_ERR_PAYLOAD_SIZE = 3
};

#endif
