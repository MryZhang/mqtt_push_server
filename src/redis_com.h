#ifndef _REDIS_COM_H_
#define _REDIS_COM_H_
#include <stdint.h>
#include "hiredis.h"

int getCtx();
int add_client_id(uint8_t *client_id);
void rm_client_id(uint8_t *client_id);
int add_topic(uint8_t *topic);
int had_client_id(uint8_t *client_id);
int clear_id_set();
int check_auth(uint8_t *username, uint8_t *password);

#endif
