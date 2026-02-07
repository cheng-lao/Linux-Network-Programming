#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>       // poll 的核心头文件
#include <errno.h>
#include <ctype.h>
#include <unistd.h>

#include "wrap.h"

#define MAXLINE 80
#define SERV_PORT 8000
#define OPEN_MAX 1024   // 初始定义的数组大小，实际可以更大

int main(int argc, char *argv[])
{
    int i, maxi, listenfd, connfd, sockfd;
    int nready;
    ssize_t n;
    char buf[MAXLINE];
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    
    // 【关键结构体数组】
    // 这里的数组大小仅受限于你的内存
    struct pollfd client[OPEN_MAX]; 

    // 1. 创建监听套接字
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    // 端口复用
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    Listen(listenfd, 128);

    // --- Poll 初始化 ---

    // client[0] 固定放监听 socket
    client[0].fd = listenfd;
    client[0].events = POLLIN; // 监听读事件

    // 初始化其余位置为 -1 (表示空闲)
    for (i = 1; i < OPEN_MAX; i++)
        client[i].fd = -1; 

    maxi = 0; // client 数组中当前使用的最大下标

    printf("Server listening on port %d...\n", SERV_PORT);

    // --- 主循环 ---
    for (;;) {
        // 2. 调用 poll
        // 参数1: 数组地址
        // 参数2: 监控的元素个数 (maxi + 1)
        // 参数3: 超时时间 (-1 永久阻塞)
        nready = poll(client, maxi + 1, -1);

        // 3. 检查监听 socket (client[0]) 是否有动静
        if (client[0].revents & POLLIN) {
            clilen = sizeof(cliaddr);
            connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
            
            char str[INET_ADDRSTRLEN];
            printf("Received from %s at PORT %d\n",
                    inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),
                    ntohs(cliaddr.sin_port));

            // 将新连接加入数组
            for (i = 1; i < OPEN_MAX; i++) {
                if (client[i].fd < 0) {
                    client[i].fd = connfd;   // 找到空位，放入新 fd
                    client[i].events = POLLIN; // 设置监听事件
                    break;
                }
            }

            if (i == OPEN_MAX) {
                fprintf(stderr, "too many clients\n");
                exit(1); 
                // 实际工程中这里应该扩容数组 (realloc)，而不是退出
            }

            if (i > maxi)
                maxi = i; // 更新最大下标

            // 如果 nready 只有 1，说明只有新连接，没有数据传输，无需往下走
            if (--nready <= 0)
                continue; 
        }

        // 4. 检查普通客户端 socket
        for (i = 1; i <= maxi; i++) {
            if ((sockfd = client[i].fd) < 0)
                continue;

            // 检查 revents (返回的事件)
            if (client[i].revents & (POLLIN | POLLERR)) {
                if ((n = read(sockfd, buf, MAXLINE)) < 0) {
                    // 出错：如果是 RST (Connection reset by peer)
                    if (errno == ECONNRESET) {
                        printf("client[%d] aborted connection\n", i);
                        close(sockfd);
                        client[i].fd = -1; // 释放位置
                    } else {
                        perror("read error");
                    }
                } else if (n == 0) {
                    // 客户端正常关闭
                    printf("client[%d] closed connection\n", i);
                    close(sockfd);
                    client[i].fd = -1; // 释放位置
                } else {
                    // 正常读取数据
                    for (int j = 0; j < n; j++)
                        buf[j] = toupper(buf[j]);
                    write(sockfd, buf, n);
                }

                // 处理完一个，计数减一，若为 0 则提前结束循环
                if (--nready <= 0)
                    break; 
            }
        }
    }
    return 0;
}