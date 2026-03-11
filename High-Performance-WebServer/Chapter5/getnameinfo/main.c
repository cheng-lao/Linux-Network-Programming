#define _POSIX_C_SOURCE 200112L
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define NI_MAXHOST 1025
#define NI_MAXSERV 32
// getnameinfo 是 协议无关 的地址到主机名/服务名的转换函数。
// 它是 getaddrinfo 的逆操作：给定一个套接字地址结构（sockaddr_in 或 sockaddr_in6），将它解析为 可读的主机名 和 服务名。
// 一个套接字结构当中有 端口号 和 IP地址
int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <hostname|IP> <service|port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct addrinfo hints, *res, *rp;
    int ret;
    char hostbuf[NI_MAXHOST];   // 标准建议大小 1025 字节
    char servbuf[NI_MAXSERV];   // 标准建议大小 32 字节

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    // IPv4 或 IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    ret = getaddrinfo(argv[1], argv[2], &hints, &res);
    if (ret != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        exit(EXIT_FAILURE);
    }
    
    printf("Results for %s:%s\n", argv[1], argv[2]);
    for (rp = res; rp != NULL; rp = rp->ai_next) {
        // 方法1：使用 getnameinfo 反向解析
        ret = getnameinfo(rp->ai_addr, rp->ai_addrlen,
                          hostbuf, sizeof(hostbuf),
                          servbuf, sizeof(servbuf),
                          NI_NUMERICHOST | NI_NUMERICSERV);
                          // 这里我们要求返回数字格式，不进行 DNS 反向查询
        if (ret != 0) {
            fprintf(stderr, "getnameinfo: %s\n", gai_strerror(ret));
            continue;
        }
        printf("  (getnameinfo) -> %s:%s\n", hostbuf, servbuf);

        // 方法2：传统 inet_ntop 方式（仅供对比）
        void *addr;
        char ip_str[INET6_ADDRSTRLEN];
        if (rp->ai_family == AF_INET) {
            addr = &((struct sockaddr_in *)rp->ai_addr)->sin_addr;
            inet_ntop(AF_INET, addr, ip_str, sizeof(ip_str));
        } else if (rp->ai_family == AF_INET6) {
            addr = &((struct sockaddr_in6 *)rp->ai_addr)->sin6_addr;
            inet_ntop(AF_INET6, addr, ip_str, sizeof(ip_str));
        } else {
            strcpy(ip_str, "unknown");
        }
        int port = (rp->ai_family == AF_INET) ?
                    ntohs(((struct sockaddr_in *)rp->ai_addr)->sin_port) :
                    ntohs(((struct sockaddr_in6 *)rp->ai_addr)->sin6_port);
        printf("  (inet_ntop)   -> %s:%d\n", ip_str, port);
        printf("  -----------------------------------\n");
    }

    freeaddrinfo(res);
    return 0;
}

