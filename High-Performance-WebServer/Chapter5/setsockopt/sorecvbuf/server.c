#include <unistd.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUF_SIZE 512

int main(int argc, char const *argv[])
{
    int lfd, cfd;
    struct sockaddr_in addr;
    char buf[BUFSIZ];
    socklen_t addr_len = sizeof(addr);
    int nread;

    if((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket error!");
        return 1;
    }
    // 设置端口复用
    int reuse = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    // 设置接收端缓冲区，内核会按照给定值的二倍进行设置
    // 但是这里即使翻倍后，接受缓冲区还是太小了，所以这里内核只能按照规定的最小大小进行设置
    // cat /proc/sys/net/ipv4/tcp_wmem 可以查看 [min default max]
    int recvbuf = 50;
    int len = sizeof(recvbuf);
    setsockopt(lfd, SOL_SOCKET, SO_RCVBUF, &recvbuf, len);
    getsockopt(lfd, SOL_SOCKET, SO_RCVBUF, &recvbuf, &len);
    printf("recv buf is %d\n", recvbuf);
    
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    // inet_pton(AF_INET, "101.132.189.102", (void*)&addr.sin_addr.s_addr);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(lfd, (struct sockaddr*)&addr, sizeof(addr)) == -1){
        perror("bind error!");
        return 1;
    }
    listen(lfd, 128);
    if((cfd = accept(lfd, (struct sockaddr*)&addr, &addr_len)) == -1){
        perror("accept error");
        return 1;
    }
    memset(buf, '\0', BUFSIZ);
    while((nread = recv(cfd, buf, BUFSIZ - 1, 0)) > 0){
        printf("Client send %d data\n", nread);
    }
    if(nread == 0){
        printf("client close connection! sleep 1s\n");
        sleep(1);
    }else{
        perror("recv error!");
    }
    close(cfd);
    close(lfd);

    return 0;
}
