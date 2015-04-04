#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "server.h"
#include "mqtt_timer.h"

int timer_init(struct util_timer_list *list)
{
    list = malloc(sizeof(struct util_timer_list));
    if(!list)
    {
        return MQTT_ERR_NOMEM;
    }

    list->head = NULL;
    list->tail = NULL;
    return MQTT_ERR_SUCCESS;
}

int add_timer(struct util_timer_list *list, struct util_timer *timer)
{
    if(!list || !timer)
    {
        return MQTT_ERR_NULL;
    }
    if(list->head == NULL)
    {
        list->head = list->tail = timer;
        return MQTT_ERR_SUCCESS;
    }

    if(timer->expire < list->head->expire)
    {
        timer->next = list->head;
        list->head->prev = timer;
        list->head = timer;
        return MQTT_ERR_SUCCESS;
    }
    

    return add_timer_after(list, timer, list->head);
}
int add_timer_after(struct util_timer_list *list, struct util_timer *timer, struct util_timer *tar)
{
    struct util_timer *tmp = tar->next;
    while(tmp != NULL || timer->expire > tmp->expire) tmp = tmp->next;
    if(tmp != NULL)
    {
        timer->prev = tmp->prev;
        tmp->prev->next = timer;
        timer->next = tmp;
        tmp->prev = timer;
    }else{
        tmp->next = timer;
        timer->prev = tmp;
        list->tail = timer;
    }

    return MQTT_ERR_SUCCESS;
}
int adjust_timer(struct util_timer_list *list, struct util_timer *timer)
{
    if(!list || !timer) return MQTT_ERR_NULL;

    struct util_timer *tmp;
    
    if(timer == list->tail) return MQTT_ERR_SUCCESS;

    if(timer->expire < timer->next->expire) return MQTT_ERR_SUCCESS;

    if(timer == list->head)
    {
        list->head->next->prev = NULL;
        list->head = list->head->next;
        timer->next = NULL;
        return add_timer(list, timer);
    }else{
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
        data->dead_clean(data);
        list->head = tmp->next;
        if(list->head)
        {
            list->head->prev = NULL;
        }
        free(tmp);
        tmp = list->head;
    }
}

void get_client_data(struct util_timer *timer, struct client_data **data)
{
    *data = (struct client_data *) (timer - sizeof(int) - sizeof(struct sockaddr_in)); 
}