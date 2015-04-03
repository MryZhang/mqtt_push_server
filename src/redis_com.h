#ifndef _REDIS_COM_H_
#define _REDIS_COM_H_
#include "hiredis.h"

int getCtx(redisContext **ctx);
int add_client_id(char *client_id);

#endif
