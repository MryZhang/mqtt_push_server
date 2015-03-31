#ifndef _UTIL_H_
#define _UTIL_H_

#define MAX_EVENT_NUM 1024
#define MAX_BUFFER_SIZE 1024

#include <sys/epoll.h>

struct server_env
{
    int epollfd;
    struct epoll_event events[MAX_EVENT_NUM];
};
int setnonblocking(int fd);
void addfd(struct server_env *env, int fd);
void reset_oneshot(struct server_env *env, int fd);
void set_fd_out(struct server_env * env, int fd, void *str);
void set_fd_in(struct server_env *env, int fd);
#endif
