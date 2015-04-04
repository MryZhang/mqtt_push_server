#ifndef _MQTT_TIMER_H_
#define _MQTT_TIMER_H_

#include <stdlib.h>
#include <time.h>
#include "client_ds.h"
#include "server.h"

struct util_timer
{
    time_t expire;
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
int remove_timer(struct util_timer_list *list, struct util_timer *timer);
void timer_tick(struct util_timer_list *list);
void get_client_data(struct util_timer *timer, struct client_data **data);
#endif
