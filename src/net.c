#include <sys/types.h>
#include <sys/socket.h>
#include "net.h"
#include <assert.h>

ssize_t mqtt_net_read(int sockfd, void* buf, size_t count)
{
    return recv(sockfd, buf, count, 0);
}
