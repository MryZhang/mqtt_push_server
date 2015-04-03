#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "redis_com.h"
#include "hiredis.h"
#include "server.h"

const char *clientsetname="clientIDs";
 
redisContext *redisCtx;
redisReply *reply;

int getCtx()
{
    redisCtx = getRedisContext();
    if(redisCtx)
    {
        return 1;
    }else{
        return 0;
    }
}

int add_client_id(uint8_t *client_id)
{
    if(redisCtx || getCtx())
    {
        reply = redisCommand(redisCtx, "SADD %s %s", clientsetname, client_id);
        /*  when reply is null, it means something wrong happend 
            Once an error is returned the context cannot be reused and you should set up a new connection */
        if(!reply) 
        {
            printf("Error when add clientid %s\n", client_id);
            printf("Error Message %s\n", redisCtx->err);
            exit(1);
        }        
        return MQTT_ERR_SUCCESS;
    }
    return MQTT_ERR_REDIS;
}

int had_client_id(uint8_t *client_id)
{
    if(redisCtx || getCtx())
    {
        reply = redisCommand(redisCtx, "SISMEMBER %s %s", clientsetname, client_id);
        if(!reply)
        {
            printf("Error when check client_id %s\n", client_id);
            printf("Error Message %s\n", redisCtx->err);
            exit(1);
        }

        if(reply->type == REDIS_REPLY_INTEGER && reply->integer == 0)
        {
            return 0;
        }

        return 1;
    }   
    
    printf("Can't connect with Redis Server\n");
    exit(1);
    return 1;
}
