#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <hostname>\n", argv[0]);
        return 1;
    }

    char *name = argv[1];
    struct hostent *host;

    // --- 1. gethostbyname: 域名 -> IP ---
    printf("--- Part 1: gethostbyname ---\n");
    host = gethostbyname(name);
    if (!host) {
        herror("gethostbyname"); // 注意：该函数错误处理用 herror 而不是 perror
        return 1;
    }

    printf("Official name: %s\n", host->h_name);

    // 打印所有别名
    for (int i = 0; host->h_aliases[i]; i++) {
        printf("Alias %d: %s\n", i + 1, host->h_aliases[i]);
    }

    // 打印所有解析出的 IP
    struct in_addr **addr_list = (struct in_addr **)host->h_addr_list;
    char first_ip[16]; // 保存第一个 IP 用于接下来的实验
    
    for (int i = 0; addr_list[i]; i++) {
        const char* ip_str = inet_ntoa(*addr_list[i]);
        printf("IP address %d: %s\n", i + 1, ip_str);
        if (i == 0) snprintf(first_ip, 16, "%s", ip_str);
    }

    printf("\n");

    // --- 2. gethostbyaddr: IP -> 域名 ---
    printf("--- Part 2: gethostbyaddr ---\n");
    struct in_addr addr;
    inet_pton(AF_INET, first_ip, &addr); // 先把字符串 IP 转为二进制

    // 参数：地址指针、长度、地址族
    host = gethostbyaddr((const char *)&addr, sizeof(addr), AF_INET);
    if (!host) {
        // 出错会设置h_errno 并不会设置errno
        herror("gethostbyaddr");
        return 1;
    }

    printf("Reverse lookup for %s: %s\n", first_ip, host->h_name);

    return 0;
}