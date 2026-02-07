#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/select.h> // select 的核心头文件

#define SERV_PORT 8888
#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    int i, j, n;
    int maxfd;          // 当前监听的最大文件描述符，传给 select 用
    int maxi;           // client[] 数组中当前最大下标，优化循环用
    int nready;         // select 返回的就绪事件数量
    int client[FD_SETSIZE]; // 自定义数组，用来保存所有已连接的客户端 socket fd
                            // FD_SETSIZE 默认为 1024

    int lfd, cfd, sockfd;   // 服务端建立listen socket
    char buf[BUF_SIZE];
    struct sockaddr_in serv_addr, clie_addr;
    socklen_t clie_addr_len;

    // fd_set 是 select 的核心数据结构（位图）
    fd_set all_set;     // 保存所有需要监控的 fd（备份用）
    fd_set read_set;    // 传给 select 的参数（会被 select 修改）

    // 1. 创建监听 Socket
    lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) {
        perror("socket error");
        exit(1);
    }

    // 端口复用（防止服务器重启时 bind 报错）
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2. 绑定地址结构
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind error");
        exit(1);
    }

    // 3. 设置监听上限
    if (listen(lfd, 128) == -1) {
        perror("listen error");
        exit(1);
    }

    printf("Server listening on port %d...\n", SERV_PORT);

    // --- Select 初始化工作 ---

    maxfd = lfd;            // 刚开始只有 lfd，所以最大 fd 就是它
    maxi = -1;              // 数组还没存东西，下标初始化为 -1

    for (i = 0; i < FD_SETSIZE; i++) {
        client[i] = -1;     // 初始化 client 数组，-1 表示该位置空闲
    }

    FD_ZERO(&all_set);      // 清空集合
    FD_SET(lfd, &all_set);  // 将监听 socket 加入监控集合

    // --- 主循环 ---
    while (1) {
        // 每次调用 select 前，都需要重新设置 read_set
        // 因为 select 是"破坏性"的，它会把没就绪的位清零
        read_set = all_set; 

        // 4. 调用 select 阻塞等待
        // 参数1：最大描述符 + 1
        // 参数2：读集合
        // 参数3：写集合 (NULL)
        // 参数4：异常集合 (NULL)
        // 参数5：超时时间 (NULL 表示永久阻塞，直到有事件)
        nready = select(maxfd + 1, &read_set, NULL, NULL, NULL);

        if (nready < 0) {
            perror("select error");
            exit(1);
        }

        // 5. 情况一：监听 socket (lfd) 有读事件 -> 说明有新连接请求
        if (FD_ISSET(lfd, &read_set)) {
            clie_addr_len = sizeof(clie_addr);
            cfd = accept(lfd, (struct sockaddr *)&clie_addr, &clie_addr_len);
            
            char client_ip[INET_ADDRSTRLEN];
            printf("New Client IP: %s, Port: %d, CFD: %d\n",
                   inet_ntop(AF_INET, &clie_addr.sin_addr, client_ip, sizeof(client_ip)),
                   ntohs(clie_addr.sin_port), cfd);

            // 将新产生的 cfd 加入 client 数组（找个空位存起来）
            for (i = 0; i < FD_SETSIZE; i++) {
                if (client[i] < 0) {
                    client[i] = cfd;
                    break;
                }
            }

            // 如果连接数达到上限（1024）
            if (i == FD_SETSIZE) {
                fputs("too many clients\n", stderr);
                exit(1);
            }

            // 将新的 cfd 加入监控集合
            FD_SET(cfd, &all_set);

            // 更新 maxfd（select 需要）
            if (cfd > maxfd) {
                maxfd = cfd;
            }

            // 更新 maxi（优化后续循环用）
            if (i > maxi) {
                maxi = i;
            }

            // 如果 nready 只有 1，说明只有 lfd 响了，没有其他客户端发数据
            // 直接进入下一次循环，减少不必要的遍历
            if (--nready == 0) {
                continue;
            }
        }

        // 6. 情况二：普通客户端 socket 有读事件 -> 说明有数据发过来了
        // 只需要遍历 0 到 maxi 的范围，不需要遍历 1024 次
        for (i = 0; i <= maxi; i++) {
            if ((sockfd = client[i]) < 0) {
                continue; // 跳过空闲位置
            }

            // 检查该 fd 是否在就绪集合中
            if (FD_ISSET(sockfd, &read_set)) {
                // 读取数据
                n = read(sockfd, buf, sizeof(buf));

                if (n == 0) {
                    // 读到 0，说明客户端关闭了连接
                    printf("Client[cfd=%d] closed connection.\n", sockfd);
                    close(sockfd);          // 关闭 socket
                    FD_CLR(sockfd, &all_set); // 从监控集合中移除
                    client[i] = -1;         // 在数组中标记为可用
                } else if (n > 0) {
                    // 业务逻辑：转大写并回写
                    for (j = 0; j < n; j++) {
                        buf[j] = toupper(buf[j]);
                    }
                    write(sockfd, buf, n);
                } else {
                    // read 返回 -1
                    perror("read error");
                    close(sockfd);
                    FD_CLR(sockfd, &all_set);
                    client[i] = -1;
                }

                // 处理完一个就绪事件，计数器减 1
                // 如果所有就绪事件都处理完了，就跳出循环，重新 select
                if (--nready == 0) {
                    break; 
                }
            }
        }
    }
    close(lfd);
    return 0;
}