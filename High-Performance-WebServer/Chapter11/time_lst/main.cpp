#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<assert.h>
#include<stdio.h>
#include<errno.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include <libgen.h> // POSIX version
#include<sys/time.h>
#include<signal.h>
#include<sys/epoll.h>

#include"LST_Timer.h"

#define FD_LIMIT 65535
#define MAX_EVENT_NUMBER 1024
#define TIMESLOT 5

static int pipefd[2];
static sort_timer_lst timer_lst;
static int epollfd = 0;

//设置文件描述符是非阻塞的
int setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);    // fcntl函数 设置文件描述符属性 开销较大
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, new_option);
    return old_option; 
}

// 将监听节点插入到epoll监听树上
void addfd(int epollfd, int fd){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;               // 非阻塞模式--水平触发
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);  // 将文件描述符添加到EPOLL树上
    setnonblocking(fd);
}

void sig_handler(int sig){
//[问题]为什么这里要保存一下 errno? 难道send过程当中会修改errno? 如果send成功会修改 errno会成功标志
// 因为这个是信号处理函数，所以需要保存一下旧值.
    int svae_errno = errno;
    int msg = sig;
    // 回忆一下 send(sockfd, buf, len发送内容大小(单位字节), 标志位)
    send(pipefd[1], (char*) &msg, 1, 0);
    // 信号处理函数不能做太复杂的操作，这里通过给双向管道的一端写上字符，方便在另一端监听到读事件，进行处理
    errno = svae_errno;
}

void addsig(int sig){
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;  // [问题] 一般都是给sa.sa_flags设置为0，这里设置成这个是什么意思
    // SA_RESTART可以保证因为信号处理函数打断的系统调用被系统自动重新执行，但不是一定能成功执行的
    sigfillset(&sa.sa_mask);    // 屏蔽所有的信号，保证处理函数的原子性
    assert(sigaction(sig, &sa, NULL) != -1);    // 注册信号处理函数
}

void timer_handler(){
    // 定时处理任务，实际上就是调用tick函数
    timer_lst.tick();
    alarm(TIMESLOT);    // 设置TIMESLOT秒之后收到SIGALARM信号
    // 收到SIGALARM信号之后 会设置timeout标志为true，之后会调用timer_handler函数
}

// 定时器回调函数，它删除非活动链接socket上的注册事件
void cb_func(client_data* user_data){
    assert(user_data);
    epoll_ctl(epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);    // 删除socket
    close(user_data->sockfd);
    printf("Close sockfd: %d\n", user_data->sockfd);
}

int main(int agrc, char* argv[]){
    if(agrc <= 2){
        printf("usage: %s ip_address port_number", basename(argv[0]));
        return 1;
    }
    
    struct sockaddr_in server_addr;
    socklen_t server_addr_len;
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    
    int ret = 0;
    bzero(& server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    inet_pton(AF_INET, ip, &server_addr.sin_addr.s_addr);
    
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    
    // 绑定服务器端地址
    ret = bind(listenfd,(sockaddr*)&server_addr,sizeof(server_addr));
    assert(ret != -1);
    // 设置监听连接数
    ret = listen(listenfd, 1024);
    assert(ret != -1);
    
    epoll_event events[MAX_EVENT_NUMBER];
    epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd);

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);
    setnonblocking(pipefd[1]);  // 设置成非阻塞的
    addfd(epollfd, pipefd[0]);  // 加入到树中，监听双向管道是否可读

    // 设置信号处理函数
    addsig(SIGALRM);
    addsig(SIGTERM);
    bool stop_server = false;

    client_data* users = new client_data[FD_LIMIT];
    bool timeout = false;
    alarm(TIMESLOT);
    
    while(!stop_server){
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0 ) && errno != EINTR){
            printf("epoll failure!\n");
            break;
        }
        for(int i = 0;i < number; i++){
            int sockfd = events[i].data.fd;
            if(sockfd == listenfd){
                // 处理链接请求
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                while(true){
                    int connfd = accept(listenfd, (sockaddr*)&client_addr, &client_addr_len);
                    if(connfd == -1){
                        if(errno == EAGAIN || errno == EWOULDBLOCK) break;
                        perror("accept error!");
                        break;
                    }
                    // 添加节点到树上，然后再添加定时器到链表上
                    addfd(epollfd, connfd);
                    users[connfd].address = client_addr;
                    users[connfd].sockfd = connfd;
                    util_timer* timer = new util_timer;
                    timer->expire = time(nullptr) + 3 * TIMESLOT;
                    timer->cv_func = cb_func;
                    timer->user_data = &users[connfd];
                    users[connfd].timer = timer;
                    timer_lst.add_timer(timer); // 添加定时器
                }

            }else if((sockfd == pipefd[0]) && (events[i].events & EPOLLIN)){
                // 处理信号事件触发
                int sig;
                char signals[1024];
                
                memset(signals, '\0', sizeof(signals));
                ret = recv(pipefd[0], signals, sizeof(signals), 0);
                if(ret == -1){
                    continue;
                }else if(ret == -1){
                    continue;
                }else{
                    for(int i = 0; i < ret; i++){
                        switch (signals[i]) {
                        case SIGALRM: {
                            /* 用timeout标记有定时任务需要处理，但是不立即处理定时任务，因为定时任务的优先级不是很高，优先处理其他任务 */
                            timeout = true;
                            break;
                        }
                        case SIGTERM: {
                            stop_server = true;
                            break;
                        }
                        default: {
                            printf("[尚未开发]接收到了其他信号----%d!\n", sig);
                            break;
                        }
                        }
                    }
                }
                // EPOLLIN 监听读事件
            }else if(events[i].events & EPOLLIN){
                // 处理客户链接上接受的数据
                while(true){
                    // 非阻塞模式循环读取出所有的数据!
                    memset(users[sockfd].buf, '\0', sizeof(users[sockfd].buf));
                    ret = recv(sockfd, users[sockfd].buf, BUFFER_SIZE - 1, 0);
                    printf("get %d bytes of client data %s from %d\n", ret, users[sockfd].buf, sockfd);
                    util_timer* timer = users[sockfd].timer;
                    
                    if(ret < 0){
                        if(errno == EAGAIN || errno == EWOULDBLOCK){
                            // 如果是因为非阻塞原因导致，直接退出
                            break;
                        }else{
                            // 如果发生了读错误，则关闭链接，并移除对应的定时器
                            cb_func(&users[sockfd]);
                            if(timer){
                                timer_lst.del_timer(timer);
                            }
                        }
                    }else if(ret == 0){
                        // 对端关闭，那么服务器端也将移除定时器并将其从树上摘下来
                        cb_func(&users[sockfd]);
                        if(timer){
                            timer_lst.del_timer(users[sockfd].timer);
                            users[sockfd].timer = nullptr;
                        }
                    }else{
                        // 如果客户端发过来的数据是可读的，那就修改其定时器
                        if(timer){
                            time_t cur = time(NULL);
                            timer->expire = cur + 3 * TIMESLOT;
                            printf("adjust timer once\n");
                            timer_lst.adjust_timer(timer);
                        }
                    }
                }
            }else{
                printf("还需要处理其他东西，但是尚未开发!\n");
            }
        }
        if(timeout){
            timer_handler();
            timeout = false;
        }
    }

    close(listenfd);
    close(pipefd[0]);
    close(pipefd[1]);
    delete [] users;
    return 0;
}
