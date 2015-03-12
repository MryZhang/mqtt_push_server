#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <assert.h>
#include <fcntl.h>
#include <pthread.h>

#include "mqtt3_protocal.h"
#include "mqtt_packet.h"

#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 1024

struct fds
{
    int epollfd;
    int sockfd;
};

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd, int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}
void reset_oneshot(int epollfd, int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

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
        int recv_len = mqtt_net_read(sockfd, (void *)&byte, 1);
        if( recv_len == 1)
        {
            packet->command = byte;
            switch(packet.command&0xF0)
            {
                case CONNECT:
                    mqtt_handle_connect(packet);
                    break;
                
                default:
                    break;
            }
        }
        /*
        int ret = recv(sockfd, buf, BUFFER_SIZE-1, 0);
        if(ret == 0)
        {
            close(sockfd);
            printf("closed the connection\n");
            break;
        }
        else if (ret < 0)
        {
            if (errno == EAGAIN)
            {
                reset_oneshot(epollfd, sockfd);
                printf("read later\n");
                break;
            }
        }
        else
        {
            int i;
            for(i = 0; i < ret; i++)
            {
                printf("0x%x", buf[i]);
            }
            printf("\nget content: %s\n", buf);
            sleep(5);
        }
        */
    }
    printf("end thread receiving data on fd: %d\n", sockfd);
}

void et(struct epoll_event* events, int number, int epollfd, int listenfd)
{
    int i;
    for(i = 0; i < number; i++)
    {
        int sockfd = events[i].data.fd;
        if( sockfd == listenfd )
        {
            struct sockaddr_in client_address;
            socklen_t addr_len = sizeof(client_address);
            int connfd = accept(listenfd, (struct sockaddr*) &client_address, &addr_len);
            addfd(epollfd, connfd);
        }
        else if (events[i].events & EPOLLIN)
        {
            pthread_t thread;
            struct fds fds_arg;
            fds_arg.epollfd = epollfd;
            fds_arg.sockfd = sockfd;
            pthread_create(&thread, NULL, conn_handler, (void *)&fds_arg);        
        }
        else
        {
            printf("something else happended \n");    
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
    
    struct epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd);

    while(1)
    {
        ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if(ret < 0)
        {
            printf("epoll failure\n");
            break;
        }
        et(events, ret, epollfd, listenfd);
    }

    close(listenfd);
    return 0;
}
