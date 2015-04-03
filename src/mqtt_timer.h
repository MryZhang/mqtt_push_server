#ifndef _MQTT_TIMER_H_
#define _MQTT_TIMER_H_

#include <stdlib.h>
#include <time.h>
#include "client_ds.h"
#include "server.h"

struct util_timer
{
    time_t expire;
    void (*cb_func)(void *client);
    struct util_timer *prev;
    struct util_timer *next;
};

struct util_timer_list
{
    struct util_timer *head;
    struct util_timer *tail;
    struct client_data *user_data;
};
int timer_init(struct util_timer_list *list);
int add_timer(struct util_timer_list *list, struct util_timer *timer);
int add_timer_after(struct util_timer_list *list, struct util_timer *timer, struct util_timer *tar);
int adjust_timer(struct util_timer_list *list, struct util_timer *timer);
void timer_tick(struct util_timer_list *list);
#endif
