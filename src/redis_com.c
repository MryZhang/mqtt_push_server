#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include "redis_com.h"
#include "hiredis.h"
#include "server.h"
#include "mqtt_message.h"
#include "log.h"

const char *clientsetname="clientIDs";
const char *topic_hash = "topic"; 

static redisContext *redisCtx;
static redisReply *reply;

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

int check_auth(uint8_t *username, uint8_t *password)
{
    assert(username != NULL && password != NULL);
    char *auth = "auth";
    if(redisCtx || getCtx())
    {
        LOG_PRINT("username [%s]", username);

        reply = redisCommand(redisCtx, "GET %s", username);
        if(!reply)
        {
            LOG_PRINT("redisError %s", redisCtx->err);        
            return -1;
        }
        if(reply->type != REDIS_REPLY_NIL)
        {
            LOG_PRINT("password in redis [%s]\n", reply->str);
            return 0;
        }else{
            return -1;
        }                
    }
}

int had_client_id(uint8_t *client_id)
{
    if(redisCtx || getCtx())
    {
        reply = redisCommand(redisCtx, "SISMEMBER %s %s", clientsetname, client_id);
        if(!reply)
        {
            printf("Error Message %s\n", redisCtx->err);
            exit(1);
        }

        if(reply->type == REDIS_REPLY_INTEGER && reply->integer == 0)
        {
            return 0;
        }

        return 1;
    }   
    
    exit(1);
    return 1;
}

void rm_client_id(uint8_t *client_id)
{
    if(redisCtx || getCtx())
    {
        reply = redisCommand(redisCtx, "SADD %s %s", clientsetname, client_id);
        if(!reply)
        {
            printf("func rm_client_id : %s\n", redisCtx->err);
        }
    }
}
int clear_id_set()
{
    if(redisCtx || getCtx())
    {
        reply = redisCommand(redisCtx, "DEL %s", clientsetname);
        if(!reply)
        {
            printf("Error: clear client set err\n");
            return -1;
        }
    }
    return 0;
}
/*
int had_topic(uint8_t *topic)
{
    if(!redisCtx && !getCtx())
    {
        printf("redisCtx is NULL\n");
        exit(1);
    }

    reply = redisCommand(redisCtx, "HEXISTS %s %s", topic_hash, t);
    if(!reply)
    {
        printf("func had_topic : %s\n", redisCtx->err);
        exit(1);
    }
    if(reply->type == REDIS_REPLY_INTEGER && reply->integer == 1)
    {
        return 1;
    }else{
        return 0;
    }
}

struct mqtt_topic* get_topic(uint8_t *t)
{
    if(!redisCtx && !getCtx())
    {
        printf("redisCtx is NULL\n");
        exit(1);
    }
    reply = redisCommand(redisCtx, "HGET %s %s", topic_hash, t);
    if(!reply || reply->type != REDIS_REPLY_STRING)
    {
        return NULL;
    }

    return (struct mqtt_topic*)reply->str;

}
struct mqtt_topic *add_topic(uint8_t *t_name)
{
    if(!redisCtx && !getCtx())
    {
        return MQTT_ERR_NULL;
    }
    struct mqtt_topic *tp;
    tp = get_topic(t);
    if(tp) return tp;
    struct mqtt_topic *t = malloc(sizeof(struct mqtt_topic));  
    if(!t)
    {
        printf("func add_topic : malloc failure\n");
        exit(1);
    }
    mqtt_string_alloc(&(t->name), t_name); 
    t->clients = NULL;
    t->msg_head = NULL;
    t->msg_buff = NULL;
    reply = redisCommand(redisCtx, "HSET %s %s %s", topic_hash, t_name, (char *)t);
   
    return t;
}
*/
