
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../wrap.h"

#define SERV_PORT 8080

int main(int argc,char* argv[]){

    int sockfd;
    sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    ssize_t n;
    socklen_t addr_len;
    char buf[BUFSIZ];
    // 可以不用绑定地址端口 
    
    // 
    bzero(&addr, sizeof(addr));
    addr.sin_port = htons(SERV_PORT);
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
    
    struct timeval tv;
    tv.tv_sec = 5;  // 5秒超时
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (fgets(buf, BUFSIZ, stdin) != NULL) {
        n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr*)&addr, sizeof(addr));
        printf("sendto: n is %ld\n", n);
        if(n == -1){
            perror("sendto error:");
            break;
        }

        addr_len = sizeof(addr);
        // n = recvfrom(sockfd, buf, BUFSIZ - 1, 0, (struct sockaddr*)&addr, &addr_len);
        n = recvfrom(sockfd, buf, BUFSIZ - 1 , 0, NULL, NULL);    // 在客户端可以给后两个参数传递NULL
        printf("recvform: n is %ld\n", n);
        if(n == -1){
            perror("recvfrom error:");
            break;
        }
        buf[n] = '\0';
        Writen(STDOUT_FILENO, buf, n);
        // Writen(STDOUT_FILENO, "\n", 1);
    }

    close(sockfd);
    return 0;
}