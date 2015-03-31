#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <assert.h>
#include <pthread.h>

#include "mqtt3_protocal.h"
#include "mqtt_packet.h"
#include "util.h"
#include "net.h"
#include "mqtt_handler.h"
#include "server.h"

void* conn_handler(void *arg)
{
    int sockfd = ((struct fds*) arg)->sockfd;
    int epollfd = ((struct fds *) arg)->epollfd;
    printf("start new thread to receive data on fd: %d\n", sockfd);
    struct mqtt_packet *packet;
    packet = (struct mqtt_packet *) malloc(sizeof(struct mqtt_packet));
    while(1)
    {
        uint8_t byte;
        int ret;
        int recv_len = mqtt_net_read(sockfd, (void *)&byte, 1);
        if( recv_len == 1)
        {
            packet->command = byte;
            packet->fd = (struct fds*)arg;

            switch(packet->command&0xFE)
            {
                case CONNECT:
                    if((ret = mqtt_handler_connect(packet)) != MQTT_ERR_SUCCESS)
                    {
                        printf("Some Err happend!\n");
                    }
                    break;
                
                default:
                    break;
            }
        }
    }
}

void et(struct server_env *env, int number, int listenfd)
{
    int i;
    for(i = 0; i < number; i++)
    {
        int sockfd = env->events[i].data.fd;
        if( sockfd == listenfd )
        {
            struct sockaddr_in client_address;
            socklen_t addr_len = sizeof(client_address);
            int connfd = accept(listenfd, (struct sockaddr*) &client_address, &addr_len);
            addfd(env, connfd);
        }
        else if (env->events[i].events & EPOLLIN)
        {
            pthread_t thread;
            struct fds fds_arg;
            fds_arg.epollfd = env->epollfd;
            fds_arg.sockfd = sockfd;
            pthread_create(&thread, NULL, conn_handler, (void *)&fds_arg);        
        }
        else if(env->events[i].events & EPOLLOUT)
        {
            struct mqtt_epoll_data *fd_data = (struct mqtt_epoll_data*) env->events[i].data.ptr;
            struct mqtt_packet *packet = (struct mqtt_packet *)fd_data->str;
            uint32_t to_process = packet->packet_len;
            int sended_len = 0; 
            while(to_process > 0)
            {
                //sended_len = send(fd_data->fd, (const void *)(packet->payload[packet->packet_len - to_process]), to_process, 0);
                sended_len = send(fd_data->fd, &(packet->payload[packet->packet_len - to_process]), to_process, 0);
                if(sended_len < 0)
                {
                    printf("Send Err\n");
                    break;
                }
                to_process -= sended_len;
            }
            set_fd_in(env, sockfd);  
            free(packet);
            free(fd_data);
        }

    }
}
int main(int argc, char **argv)
{
    const char *ip = "127.0.0.1";
    const int port = 3308;

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    
    ret = bind(listenfd, (struct sockaddr*) &address, sizeof(address));
    assert(ret != -1);
    
    ret = listen(listenfd, 5);
    assert(ret != -1);
    
    struct server_env env;
    env.epollfd = epoll_create(5);
    assert(env.epollfd != -1);
    addfd(&env, listenfd);

    while(1)
    {
        ret = epoll_wait(env.epollfd, env.events, MAX_EVENT_NUM, -1);
        if(ret < 0)
        {
            printf("epoll failure\n");
            break;
        }
        et(&env, ret, listenfd);
    }

    close(listenfd);
    return 0;
}
