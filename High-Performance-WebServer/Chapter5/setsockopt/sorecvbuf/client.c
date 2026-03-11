#include <unistd.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUF_SIZE 512

int main(int argc, char const *argv[])
{
    int cfd;
    struct sockaddr_in addr;
    char buf[BUFSIZ];

    if((cfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket error!");
        return 1;
    }
    
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "101.132.189.102", (void*)&addr.sin_addr.s_addr);
    
    // 设置客户端发送缓冲区大小
    // 设置接收端缓冲区，内核会按照给定值的二倍进行设置
    // 但是这里即使翻倍后，接受缓冲区还是太小了，所以这里内核只能按照规定的最小大小进行设置
    // cat /proc/sys/net/ipv4/tcp_wmem 可以查看 [min default max]
    int sendbuf = 2048;
    int len = sizeof(sendbuf);
    setsockopt(cfd, SOL_SOCKET, SO_SNDBUF, &sendbuf, sizeof(sendbuf));
    getsockopt(cfd, SOL_SOCKET, SO_SNDBUF, &sendbuf, &len);
    printf("sendbuf is %d\n", sendbuf);

    if(connect(cfd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        perror("connect error");
        return 1;
    }

    // 客户端发送数据
    memset(buf, 'a', BUF_SIZE);
    send(cfd, buf, BUF_SIZE, 0);
    
    // sleep(5);

    close(cfd);

    return 0;
}
