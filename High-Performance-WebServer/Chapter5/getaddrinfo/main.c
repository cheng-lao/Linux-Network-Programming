#define _POSIX_C_SOURCE 200112L
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char const *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <hostname/IP> <service/port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct addrinfo hints;
    struct addrinfo *res, *rp;
    int ret;
    char ip_str[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;      // 允许 IPv4/IPv6
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_flags = AI_CANONNAME;

    ret = getaddrinfo(argv[1], argv[2], &hints, &res);
    if (ret != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        exit(EXIT_FAILURE);
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        // 获取可读的地址字符串
        void *addr;
        if (rp->ai_family == AF_INET) {
            addr = &((struct sockaddr_in *)rp->ai_addr)->sin_addr;
        } else if (rp->ai_family == AF_INET6) {
            addr = &((struct sockaddr_in6 *)rp->ai_addr)->sin6_addr;
        } else {
            continue;   // 其他协议族跳过
        }
        inet_ntop(rp->ai_family, addr, ip_str, sizeof(ip_str));

        // 打印规范名（可能为 NULL）
        printf("canonname: %s, ip: %s\n",
               rp->ai_canonname ? rp->ai_canonname : "(none)",
               ip_str);
    }

    freeaddrinfo(res);
    return 0;
}