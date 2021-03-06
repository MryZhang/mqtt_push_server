#include <sys/epoll.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>

#include "util.h"
#include "net.h"
#include "log.h"

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(struct server_env *env, int fd, int oneshot)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    if(oneshot)
    {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(env->epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void reset_oneshot(struct server_env *env, int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(env->epollfd, EPOLL_CTL_MOD, fd, &event);
}

void set_fd_out(struct server_env *env, int fd, void *str, int len)
{
    struct epoll_event event;
    struct mqtt_epoll_data *fd_str;
    fd_str = (struct mqtt_epoll_data *) malloc(sizeof(struct mqtt_epoll_data));
    assert(fd_str);
    fd_str->fd = fd;
    fd_str->str = str;
    fd_str->len = len;
    event.data.ptr = fd_str;
    event.events = EPOLLOUT | EPOLLET;
    epoll_ctl(env->epollfd, EPOLL_CTL_MOD, fd, &event);
}

void set_fd_in(struct server_env *env, int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(env->epollfd, EPOLL_CTL_MOD, fd, &event);
}

void removefd(struct server_env *env, int fd)
{
    LOG_PRINT("In function removefd");
    epoll_ctl(env->epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
    LOG_PRINT("In end of function removefd");
}

