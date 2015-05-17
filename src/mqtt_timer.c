#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "server.h"
#include "mqtt_timer.h"
#include "log.h"

int timer_init(struct util_timer_list *list)
{

    list->head = NULL;
    list->tail = NULL;
    return MQTT_ERR_SUCCESS;
}

int add_timer(struct util_timer_list *list, struct util_timer *timer)
{
    LOG_PRINT("add the timer");
    if(!list || !timer)
    {
        return MQTT_ERR_NULL;
    }
    LOG_PRINT("A");
    if(list->head == NULL)
    {
    LOG_PRINT("A");
        list->head = timer;
        list->tail = timer;
    LOG_PRINT("A");
        return MQTT_ERR_SUCCESS;
    }
    LOG_PRINT("A");

    if(timer->expire < list->head->expire)
    {
    LOG_PRINT("A");
        timer->next = list->head;
        list->head->prev = timer;
        list->head = timer;
    LOG_PRINT("A");
        return MQTT_ERR_SUCCESS;
    }
    
    LOG_PRINT("A");

    return add_timer_after(list, timer, list->head);
}
int add_timer_after(struct util_timer_list *list, struct util_timer *timer, struct util_timer *tar)
{
    assert(tar != NULL);
    LOG_PRINT("A");
    struct util_timer *tmp = tar;
    LOG_PRINT("A");
    while(tmp->next != NULL && timer->expire > tmp->next->expire) tmp = tmp->next;
    LOG_PRINT("A");
    if(tmp->next == NULL)
    {
        timer->prev = tmp;
        timer->next = tmp->next;
        tmp->next = timer;
        list->tail = timer;
    }else{
        timer->prev = tmp;
        tmp->next->prev = timer;
        timer->next = tmp->next;
        tmp->next  = timer;
    }

    LOG_PRINT("A");
    return MQTT_ERR_SUCCESS;
}
int adjust_timer(struct util_timer_list *list, struct util_timer *timer)
{
    if(!list || !timer) return MQTT_ERR_NULL;
    LOG_PRINT("A");
    struct util_timer *tmp;
    LOG_PRINT("A");
    
    if(timer == list->tail) return MQTT_ERR_SUCCESS;
    LOG_PRINT("A");

    if(timer->expire < timer->next->expire) 
    {
        LOG_PRINT("ddd");
        return MQTT_ERR_SUCCESS;
    }
    LOG_PRINT("A");

    if(timer == list->head)
    {
    LOG_PRINT("A");
        list->head->next->prev = NULL;
        list->head = list->head->next;
        timer->next = NULL;
        return add_timer(list, timer);
    }else{
    LOG_PRINT("A");
        timer->next->prev =timer->prev;
        timer->prev->next = timer->next;
        tmp = timer->next; 
        timer->prev = timer->next = NULL;
        return add_timer_after(list, timer, tmp);
    }
}

int remove_timer(struct util_timer_list *list, struct util_timer *timer)
{
    if(!list || !list->head || timer) return MQTT_ERR_NULL;
    if(timer == list->head)
    {
        list->head = timer->next;
        list->head->prev = NULL;
    }
    else if(timer == list->tail)
    {
        list->tail = timer->prev;
        list->tail->next = NULL;
    }
    else
    {
        timer->next->prev = timer->prev;
        timer->prev->next = timer->next;
    }
    return MQTT_ERR_SUCCESS;
}

void timer_tick(struct util_timer_list *list)
{
    if(!list || !list->head) return;

    time_t cur = time(NULL);
    struct util_timer *tmp = list->head;
    while(tmp)
    {
        if(cur < tmp->expire)
        {
            break;
        }
        struct client_data *data;
        get_client_data(tmp, &data);
        assert(data);
        assert(data->dead_clean);
        assert(data->sockfd);
        LOG_PRINT("client [%d] time expire", data->sockfd);
        data->dead_clean(data->sockfd);
        list->head = tmp->next;
        if(list->head)
        {
            list->head->prev = NULL;
        }
        tmp = list->head;
    }
}

void get_client_data(struct util_timer *timer, struct client_data **data)
{
    *data = (struct client_data *) (timer); 
}

int inc_timer(struct util_timer_list *list, struct util_timer *timer)
{
    LOG_PRINT("Update the timer");
    timer->expire = time(NULL) + 24; 
    return adjust_timer(list, timer);
}
