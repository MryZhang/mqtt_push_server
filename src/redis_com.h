#ifndef _REDIS_COM_H_
#define _REDIS_COM_H_
#include <stdint.h>
#include "hiredis.h"

int getCtx();
int add_client_id(uint8_t *client_id);
void rm_client_id(uint8_t *client_id);
int add_topic(uint8_t *topic);
int

#endif
