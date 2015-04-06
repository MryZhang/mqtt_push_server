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
#include <time.h>
#include <signal.h>

#include "mqtt3_protocal.h"
#include "mqtt_packet.h"
#include "util.h"
#include "net.h"
#include "mqtt_handler.h"
#include "server.h"
#include "hiredis.h"

static int pipefd[2];
static redisContext *redis_context;
static redisReply *reply;
static struct server_env *env;

struct server_env *get_server_env()
{
    return env;
}

redisContext *getRedisContext()
{
    return redis_context;
}

void sig_handler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

void addsig(int sig)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL)!= -1);
}

void timer_handler()
{
    assert(env != NULL);
    timer_tick(env->timer_list);
    alarm(TIMESLOT);
}

void* conn_handler(void *arg)
{
    int sockfd = ((struct fds*) arg)->sockfd;
    int epollfd = ((struct fds *) arg)->epollfd;
    int readed = 0;
    printf("start new thread to receive data on fd: %d\n", sockfd);
    struct mqtt_packet *packet;
    packet = (struct mqtt_packet *) malloc(sizeof(struct mqtt_packet));
    
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
                    printf("mqtt_handler_connect failure. Errcode: %d\n", ret);
                    shut_dead_conn(packet->fd->sockfd);
                }
                break;        
            case PUBLISH:
                mqtt_handler_publish(packet);
                break;  
            case PINGREQ:
                break; 
            default:
                break;
        }    
    }else if(recv_len == 0)
    {
        shut_dead_conn(sockfd);
    }
    struct server_env *env = get_server_env();
    assert(env != NULL);
    reset_oneshot(env, sockfd);
}

void et(struct server_env *env, int number, int listenfd)
{
    int i, ret;
    for(i = 0; i < number; i++)
    {
        int sockfd = env->events[i].data.fd;
        if( sockfd == listenfd )
        {
            printf("New Client=====================\n");
            struct sockaddr_in client_address;
            socklen_t addr_len = sizeof(client_address);
            int connfd = accept(listenfd, (struct sockaddr*) &client_address, &addr_len);
            env->clients[connfd].address = client_address;
            env->clients[connfd].sockfd = connfd;
            addfd(env, connfd, 1);

            time_t cur = time(NULL);
            env->clients[connfd].timer.expire = cur + 5*TIMESLOT;
            env->clients[connfd].dead_clean = shut_dead_conn;
            printf("client address: 0x%x\n", &(env->clients[connfd]));
            printf("timer address: 0x%x\n", &(env->clients[connfd].timer));
            assert(add_timer(env->timer_list, &(env->clients[connfd].timer))== MQTT_ERR_SUCCESS);
        }else if(sockfd == pipefd[0] && (env->events[i].events & EPOLLIN))
        {
            printf("ALARM========================\n");
            int sig;
            char signals[1024];
            ret = recv(pipefd[0], signals, sizeof(signals), 0);
            if(ret == -1)
            {
                continue;
            }else if(ret == 0)
            {
                continue;    
            }else
            {
                int i;
                for(i = 0; i < ret; i++)
                {
                    switch(signals[i])
                    {
                    case SIGALRM:
                        timer_handler();
                        break;
                    case SIGINT:
                        printf("Goodbye!\n");
                        exit(1);
                        break;
                    default:
                        break;                        
                    }
                }
            }
        }else if (env->events[i].events & EPOLLIN)
        {
            printf("EPOLLIN=================\n");
            pthread_t thread;
            struct fds fds_arg;
            fds_arg.epollfd = env->epollfd;
            fds_arg.sockfd = sockfd;
            pthread_create(&thread, NULL, conn_handler, (void *)&fds_arg);        
        }
        else if(env->events[i].events & EPOLLOUT)
        {
            printf("EPOLLOUT===============\n");
            struct mqtt_epoll_data *fd_data = (struct mqtt_epoll_data*) env->events[i].data.ptr;
            uint32_t to_process = fd_data->len;
            int sended_len = 0; 
            int send_ret = 0;
            while(to_process > 0)
            {
                send_ret = send(fd_data->fd, &(((uint8_t *)fd_data->str)[sended_len]), to_process, 0);
                if(send_ret > 0)
                {
                    to_process = to_process - send_ret;
                    sended_len = sended_len + send_ret;
                }
                else
                {
                    printf("func et: send ret != 0\n");
                    break;   
                }
            }
            set_fd_in(env, sockfd);  
        }

    }
}
int main(int argc, char **argv)
{
    const char *ip = "127.0.0.1";
    const int port = 3310;

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    const char *redis_host = "127.0.0.1";
    int redis_port = 6379;

    struct timeval redis_timeout = {1, 500000};
    redis_context = redisConnectWithTimeout(redis_host, redis_port, redis_timeout);
    if(redis_context == NULL || redis_context->err)
    {
        if(redis_context)
        {
            printf("Connection error: %s\n", redis_context->errstr);
            redisFree(redis_context);
        }else{
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }

    
    
    addsig(SIGALRM);
    addsig(SIGTERM);
    //addsig(SIGINT);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    
    int on = 1;
    ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    assert(ret == 0);

    ret = bind(listenfd, (struct sockaddr*) &address, sizeof(address));
    assert(ret != -1);
    
    ret = listen(listenfd, 5);
    assert(ret != -1);
    
    env = malloc(sizeof(struct server_env));
    if(!env)
    {
        perror("malloc");
        exit(1);
    }
    //initiate the timer list
    env->timer_list = malloc(sizeof(struct util_timer_list));
    assert(env->timer_list);
    if( mqtt_hash_init(&(env->topic_table))!=0)
    {
        perror("malloc");
        exit(1);
    }
    if(timer_init(env->timer_list) != MQTT_ERR_SUCCESS)
    {
        printf("Timer list init failure\n");
        exit(1);
    }
    env->epollfd = epoll_create(5);
    assert(env->epollfd != -1);
    addfd(env, listenfd, 0);

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);
    setnonblocking(pipefd[1]);
    addfd(env, pipefd[0], 0);

    alarm(TIMESLOT);
    while(1)
    {
        ret = epoll_wait(env->epollfd, env->events, MAX_EVENT_NUM, 100);
        if(ret < 0)
        {
            perror("epoll_wait:");
            continue;
        }
        et(env, ret, listenfd);
    }

    close(listenfd);
    return 0;
}
