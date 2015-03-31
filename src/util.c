#include <sys/epoll.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>

#include "util.h"
#include "net.h"

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(struct server_env *env, int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
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

void set_fd_out(struct server_env *env, int fd, void *str)
{
    struct epoll_event event;
    struct mqtt_epoll_data *fd_str;
    fd_str = (struct mqtt_epoll_data *) malloc(sizeof(struct mqtt_epoll_data));
    fd_str->fd = fd;
    fd_str->str = str;
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
