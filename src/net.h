#ifndef _NET_H_
#define _NET_H_

#include <sys/types.h>
#include <stdio.h>

#include "mqtt_packet.h"

struct mqtt_epoll_data
{
    int fd;
    void *str;
    int len;
};
struct fds
{
    int epollfd;
    int sockfd;
};
ssize_t mqtt_net_read(int sockfd, void *buf, size_t count);
#endif
