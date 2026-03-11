#include <assert.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
/*
如果说 gethostbyname 是网络编程中的“电话簿”（查找人的地址），那么 getservbyname 和 getservbyport 就是“行业分类黄页”。
它们的作用是让你的程序不再死记硬背端口号（如 80、443），而是通过服务名称（如 "http", "https"）来获取端口信息，或者反过来查询。
*/
// ... 头文件 ...

int main(int argc, char const *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hostname>\n", argv[0]);
        exit(1);
    }

    struct hostent* hostinfo = gethostbyname(argv[1]);
    if (!hostinfo) {
        fprintf(stderr, "gethostbyname error: %s\n", hstrerror(h_errno));
        exit(1);
    }

    struct servent* servinfo = getservbyname("daytime", "tcp");
    if (!servinfo) {
        fprintf(stderr, "getservbyname: daytime/tcp not found\n");
        exit(1);
    }

    // 打印服务信息（略）

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = servinfo->s_port;  // 已是网络字节序
    // hostinfo->h_addr_list也是网络字节序 不需要再使用inet_pton转换
    memcpy(&addr.sin_addr.s_addr, hostinfo->h_addr_list[0], hostinfo->h_length);

    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd == -1) {
        perror("socket");
        exit(1);
    }

    if (connect(cfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(cfd);
        exit(1);
    }

    char buf[BUFSIZ];
    int res = recv(cfd, buf, sizeof(buf) - 1, 0);
    if (res <= 0) {
        if (res == 0)
            fprintf(stderr, "Connection closed by server.\n");
        else
            perror("recv");
        close(cfd);
        exit(1);
    }

    buf[res] = '\0';
    printf("The daytime is: %s", buf);
    close(cfd);
    return 0;
}
