#ifndef _MQTT_HANDLER_H_
#define _MQTT_HANDLER_H_
#include <stdio.h>
#include "mqtt_packet.h"

int mqtt_handle_connect(struct mqtt_packet * packet);

#endif
