#ifndef _NET_H_
#define _NET_H_

#include <sys/types.h>
#include <stdio.h>


ssize_t mqtt_net_read(int sockfd, void *buf, size_t count);

#endif
