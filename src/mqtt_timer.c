#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "server.h"
#include "mqtt_timer.h"
#include "log.h"

int timer_init(struct util_timer_list *list)
{

    list->head = NULL;
    list->tail = NULL;
    pthread_mutex_init(&(list->mutex), NULL);
    return MQTT_ERR_SUCCESS;
}

int add_timer(struct util_timer_list *list, struct util_timer *timer)
{
    int ret;
    LOG_PRINT("Add the timer");
    if(!list || !timer)
    {
        return MQTT_ERR_NULL;
    }
    assert(pthread_mutex_lock(&(list->mutex))==0); 
    if(list->head == NULL)
    {
        list->head = timer;
        list->tail = timer;
        assert(pthread_mutex_unlock(&(list->mutex)) == 0);
        return MQTT_ERR_SUCCESS;
    }

    if(timer->expire < list->head->expire)
    {
        timer->next = list->head;
        list->head->prev = timer;
        list->head = timer;
        assert(pthread_mutex_unlock(&(list->mutex)) == 0);
        return MQTT_ERR_SUCCESS;
    }
    
    ret = add_timer_after(list, timer, list->head);
    assert(pthread_mutex_unlock(&(list->mutex))==0);
    return ret;
}
int add_timer_after(struct util_timer_list *list, struct util_timer *timer, struct util_timer *tar)
{
    assert(tar != NULL);
    struct util_timer *tmp = tar;
    while(tmp->next != NULL && timer->expire > tmp->next->expire) tmp = tmp->next;
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

    return MQTT_ERR_SUCCESS;
}
int adjust_timer(struct util_timer_list *list, struct util_timer *timer)
{
    if(!list || !timer) return MQTT_ERR_NULL;
    struct util_timer *tmp;
    
    if(timer == list->tail) return MQTT_ERR_SUCCESS;

    if(timer->expire < timer->next->expire) 
    {
        return MQTT_ERR_SUCCESS;
    }

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
    LOG_PRINT("In funciton remove_timer");
    if(!list || !list->head || !timer) return MQTT_ERR_NULL;
    LOG_PRINT("Remove timer");
    pthread_mutex_lock(&(list->mutex));
    if(timer == list->head)
    {
        list->head = timer->next;
        if(list->head != NULL){
            list->head->prev = NULL;
        }else{
            list->tail = NULL; 
        }
    }
    else if(timer == list->tail)
    {
        list->tail = timer->prev;
        list->tail->next = NULL;
    }
    else
    {
        if(timer->next != NULL && timer->prev != NULL)
        {
            timer->next->prev = timer->prev;
            timer->prev->next = timer->next;
        }
    }
    timer->next = NULL;
    timer->prev = NULL;
    pthread_mutex_unlock(&(list->mutex));
    LOG_PRINT("Timer is Removed");
    return MQTT_ERR_SUCCESS;
}

void timer_tick(struct util_timer_list *list)
{
    if(!list || !list->head) return;
    LOG_PRINT("Timer tick!");
    time_t cur = time(NULL);
    pthread_mutex_lock(&(list->mutex));
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
        LOG_PRINT("A");
        list->head = tmp->next;
        LOG_PRINT("A");
        if(list->head)
        {
            LOG_PRINT("A");
            list->head->prev = NULL;
        }else{
            LOG_PRINT("A");
            list->tail = NULL;
        }
        LOG_PRINT("A");
        tmp = list->head;
        LOG_PRINT("A");
    }
    pthread_mutex_unlock(&(list->mutex));
    LOG_PRINT("Timer no expire");
}

void get_client_data(struct util_timer *timer, struct client_data **data)
{
    *data = (struct client_data *) (timer); 
}

int inc_timer(struct util_timer_list *list, struct util_timer *timer, uint16_t inctime)
{
    int ret;
    LOG_PRINT("Update the timer");
    pthread_mutex_lock(&(list->mutex));
    timer->expire = time(NULL) + inctime; 
    ret = adjust_timer(list, timer);
    pthread_mutex_unlock(&(list->mutex));
    return ret;
}
