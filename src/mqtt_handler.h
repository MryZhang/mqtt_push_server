#ifndef _MQTT_HANDLER_H_
#define _MQTT_HANDLER_H_
#include <stdio.h>
#include "mqtt_packet.h"
#include "client_ds.h"

int mqtt_handler_connect(struct mqtt_packet *packet);
int mqtt_conn_ack(struct mqtt_packet *packet, int ret_code);
void shut_dead_conn(struct client_data *client);
#endif
