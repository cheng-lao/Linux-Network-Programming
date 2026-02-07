#include "../wrap.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERV_PORT 8080

int main(int argc,char* argv[]){

    int sockfd;
    ssize_t n;
    struct sockaddr_in addr;
    socklen_t addr_len;
    char buf[BUFSIZ];
    char ip_str[INET_ADDRSTRLEN];
    
    //1.创建socket
    sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

    //2. 设置服务端地址结构
    bzero(&addr, sizeof(addr));
    addr.sin_port = htons(SERV_PORT);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    //3. 绑定端口，不用设置listen数量，因为是无连接的
    Bind(sockfd, (struct sockaddr*)&addr,  sizeof(addr));
    
    while (1) {
        addr_len = sizeof(addr);
        n = recvfrom(sockfd, buf, BUFSIZ - 1, 0, (struct sockaddr*)&addr, &addr_len);
        if (n == -1) {
            perror("recvfrom error:");
            break;
        }
        printf("ip: %s, port: %d send msg!\n",
             inet_ntop(AF_INET, (void*)&addr.sin_addr.s_addr, ip_str, INET_ADDRSTRLEN),
            ntohs(addr.sin_port));

        buf[n] = '\0';
        for (int i = 0; i < n; i++) {
            buf[i] = toupper(buf[i]);
        }
        n = sendto(sockfd, buf, strlen(buf), 0, 
        (struct sockaddr*)&addr, addr_len);
        if (n == -1) {
            perror("sendto error:");
            break;
        }
    }

    close(sockfd);
    return 0;
}