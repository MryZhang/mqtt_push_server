#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>

#include "net.h"

ssize_t mqtt_net_read(int sockfd, void* buf, size_t count)
{
    return recv(sockfd, buf, count, 0);
}
