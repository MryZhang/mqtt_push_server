#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "server.h"
#include "mqtt_timer.h"

int timer_init(struct util_timer_list *list)
{

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
    printf("Timer tick ====================\n");
    if(!list || !list->head) return;

    printf("Start to tick:\n");
    time_t cur = time(NULL);
    struct util_timer *tmp = list->head;
    while(tmp)
    {
        if(cur < tmp->expire)
        {
            break;
        }
        printf("Found a expired timer============\n");
        struct client_data *data;
        get_client_data(tmp, &data);
        printf("Call back to shut dead conn\n");
        assert(data);
        assert(data->dead_clean);
        assert(data->sockfd);
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
    printf("func : get_client_data timer address 0x%x\n", timer);
    *data = (struct client_data *) (timer); 
    printf("func : get_client_data client_data address 0x%x\n", *data);
}

int inc_timer(struct util_timer_list *list, struct util_timer *timer)
{
    timer->expire += 24; 
    return adjust_timer(list, timer);
}
