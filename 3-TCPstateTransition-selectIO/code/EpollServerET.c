
#include "wrap.h"
#include <arpa/inet.h>
// #include <cfloat>
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_EVENTS 1024

void set_nonblocking(int fd){
    int flags = fcntl(fd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
}

//设置边沿触发ET, 必须搭配客户端为非阻塞模式
int main(int argc, char* argv[]){

    int listenfd, clientfd;
    struct sockaddr_in addr;
    socklen_t addr_len;
    int opt = 1, nread;
    char ip_str[INET_ADDRSTRLEN];
    char buf[BUFSIZ];

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    
    bzero(&addr, sizeof(addr));
    addr.sin_port = htons(8080);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));  //设置端口复用
    Bind(listenfd, (struct sockaddr*)&addr, sizeof(addr));
    printf("Bind IP: %s, Port: %d\n", 
        inet_ntop(AF_INET, &addr.sin_addr.s_addr, ip_str, INET_ADDRSTRLEN),
        ntohs(addr.sin_port));

    Listen(listenfd, 128);

    int epfd = epoll_create(MAX_EVENTS);
    struct epoll_event ep[MAX_EVENTS], tmpep;
    tmpep.events = EPOLLIN;                     //更新初始化 listenfd
    tmpep.data.fd = listenfd;

    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &tmpep);
    while (1) {
        nread = epoll_wait(epfd, ep, MAX_EVENTS, -1);
        if(nread == -1){
            sys_err("epoll_wait error:");
        }else if(nread == 0) continue;
        
        for (int i = 0; i < nread; i ++) {
            if(ep[i].data.fd == listenfd){
                //有链接建立
                addr_len = sizeof(addr);
                clientfd = Accept(listenfd, (struct sockaddr*)&addr, &addr_len);
                printf("Connect Client------IP: %s, Port: %d\n", 
                    inet_ntop(AF_INET, &addr.sin_addr.s_addr, ip_str, INET_ADDRSTRLEN),
                    ntohs(addr.sin_port));
                
                tmpep.events = EPOLLIN | EPOLLET;     //更新初始化epoll_event (设置为ET边沿触发)默认是水平触发，这里添加上EPOLLET即为边沿触发
                tmpep.data.fd = clientfd;
                set_nonblocking(clientfd);      //设置成非阻塞模式
                epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &tmpep);
            }else{
                clientfd = ep[i].data.fd;
                // int n = Readn(clientfd, buf, BUFSIZ);   //设置为非阻塞之后，readn不会阻塞会立即返回，之前封装的Readn函数不能再使用了
                while (1) {
                    int n = read(clientfd, buf, BUFSIZ);
                    if(n == 0){
                        //关闭链接
                        printf("Close Client---fd: %d\n", clientfd);
                        close(clientfd);    //close 后，epoll 会自动将其从树上移除 
                        break;
                    }else if(n > 0){
                        for (int j = 0; j < n; j++) {
                            if (buf[j] >= 'a' && buf[j] <= 'z')
                                buf[j] = toupper(buf[j]);
                        }
                        Writen(clientfd, buf, n);
                        Writen(STDOUT_FILENO, buf, n);
                        Writen(STDOUT_FILENO, "\n", 1);
                        break;      // 跳出读写循环
                    }else {
                        if(n == EAGAIN || n == EWOULDBLOCK){
                            break;
                            // 在 ET 模式下，必须循环读取直至 EAGAIN
                        }  // 非阻塞模式下可以使用
                        perror("read error!");
                        close(clientfd);
                        epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, NULL);
                        break;
                    }
                }
            }
        }
    }
    close(listenfd);
    return 0;
}