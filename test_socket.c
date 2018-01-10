#include "coroutine.h"
#include <stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#define MAXLINE 1024
#define PORT 8888
// typedef struct _ConnfdList
// {
//     int len;
//     int max_index;
//     int *fds;
// }ConnfdList;

#define PEER_IP     "0.0.0.0"  

int create_socket()
{
    int sock_fd;
    if( (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){  
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);  
        return -1;
    }  
    int flags = fcntl(sock_fd, F_GETFL, 0);  
    fcntl(sock_fd, F_SETFL, flags|O_NONBLOCK);
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));  
    servaddr.sin_family = AF_INET;  
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);//IP地址设置成INADDR_ANY,让系统自动获取本机的IP地址。  
    servaddr.sin_port = htons(PORT);//设置的端口为DEFAULT_PORT

    if( bind(sock_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){  
        printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
    }  
    //开始监听是否有客户端连接  
    if( listen(sock_fd, 1024) == -1){  
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
    } 
    return sock_fd;
}

void accept_conn(int sock_fd, schedule *s, int corourine_ids[], coroutine_func handle)
{
    int connfd = 0;
    while(1)
    {
        // printf("accept!\n");
        // connfd = accept(sock_fd,(struct sockaddr*)NULL,NULL);
        int flags = fcntl(sock_fd, F_GETFL, 0);  
        fcntl(sock_fd, F_SETFL, flags|O_NONBLOCK);
        connfd = accept(sock_fd,(struct sockaddr*)NULL,NULL);
        if(connfd > 0)
        {
            puts("conn\n");
            int args[] = {sock_fd, connfd};
            int id = coroutine_create(s, handle, (void *)args);
            int i=0;
            for(i=0; i<MAX_UTHREAD_SIZE; i++)
            {
                int cid = corourine_ids[i];
                if(cid == -1)
                {
                    corourine_ids[i] = id;
                    break;
                }else if(i == MAX_UTHREAD_SIZE - 1){
                    puts("overflow of ids");
                }
            }
            coroutine_running(s, id);
        }else
        {
            // 恢复所有
            int i=0;
            for(i=0; i<MAX_UTHREAD_SIZE; i++)
            {
                int cid = corourine_ids[i];
                if(cid == -1)
                {
                    continue;
                }
                coroutine_resume(s, cid);
            }
            //coroutine_resume(s, id);
        }
    }
}

void handle_conn(schedule *s, void *args)
{
    int *arg_arr = (int *)args;
    int connfd = arg_arr[1];
    int n = 0;
    char buff[MAXLINE] = {0};
    // puts("handle\n");
    while(1)
    {
        // printf("while:%d\n", connfd);
        n = recv(connfd, buff, 10, MSG_DONTWAIT);
        // printf("recv_n:%d\n", n);
        if(n <= 0)
        {
            coroutine_yield(s);
        }else {
            printf("connfd:%d,recv_data:%s\n", connfd, buff);
            int send_len = send(connfd, buff, strlen(buff), MSG_DONTWAIT);
            memset(buff, 0, MAXLINE);
            if(strcmp(buff, "exit") == 0)
            {
                break;
            }
        }
    }
    close(connfd);
}

int main()
{
    int corourine_ids[MAX_UTHREAD_SIZE] = {-1};
    int i=0;
    for(i=0; i<MAX_UTHREAD_SIZE; i++)
    {
        corourine_ids[i] = -1;
    }
    int sock_fd = create_socket();
    schedule *s = schedule_create();
    accept_conn(sock_fd, s, corourine_ids, handle_conn);
    printf("over main");
    schedule_close(s);
    close(sock_fd);
    return 0;
}