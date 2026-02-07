#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <stdlib.h>
#include <ctype.h>
#include <netinet/in.h>
#include <stdio.h>
// #include <string.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "wrap.h"

int main(int argc, char* argv[]){

    struct sockaddr_in addr;
    socklen_t addr_len;
    int listenfd, clientfd;
    char ip_str[INET_ADDRSTRLEN];
    int client[FD_SETSIZE];
    char buf[BUFSIZ];

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    bzero(&addr, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr =  htonl(INADDR_ANY);
    Bind(listenfd, (struct sockaddr*)&addr, sizeof(addr));
    printf("Server bind IP: %s, port: %d\n",
        inet_ntop(AF_INET, &addr.sin_addr.s_addr, ip_str, INET_ADDRSTRLEN),
        ntohs(addr.sin_port));
    
    Listen(listenfd, 128);

    fd_set allset, rset;
    int maxfd = listenfd, ret, maxi = -1, i = 0;
    for (i = 0; i < FD_SETSIZE; i++) {
        client[i] = -1;              // 初始化client数组，数组内存储每个客户端的cfd
    }

    FD_ZERO(&allset);    
    FD_SET(listenfd, &allset);          // 清空

    while(1){
        rset = allset;
        // select保证当事件来临的时候可以返回通知客户端，客户端后续做的事情都不会发生阻塞
        ret = select(maxfd + 1, &rset, NULL, NULL, NULL);   //阻塞模式
        if(ret == -1){
            sys_err("select error");    
        }
        if(FD_ISSET(listenfd, &rset)){
            //开始监听
            addr_len = sizeof(addr);
            clientfd = Accept(listenfd,(struct sockaddr*)&addr, &addr_len); // 监听事件发生 不会阻塞
            printf("Connect Client IP: %s, Port: %d\n", 
                inet_ntop(AF_INET, &addr.sin_addr.s_addr, ip_str, sizeof(ip_str)),
                ntohs(addr.sin_port));
            
                
            for (i = 0; i < FD_SETSIZE; i++) {
                if (client[i] < 0) {
                    client[i] = clientfd;
                    break;
                }
            }
            if(i == FD_SETSIZE){
                close(clientfd);    // 人满拒绝
                continue;
            }
                
            FD_SET(clientfd, &allset);  //添加客户端文件描述符
            if(i > maxi) maxi = i;
            if(clientfd > maxfd) maxfd = clientfd;       //更新最大的文件描述符
            if(ret == 1) continue;
        }
        for(i = 0;i <= maxi; i++){
            int cfd = client[i];
            if(cfd == -1) continue;
            
            if(FD_ISSET(cfd, &rset)){//有客户端进行写数据
                int n = Readn(cfd, buf, sizeof(buf)); // 处理读事件 不会阻塞
                if (n == 0) {
                    //客户端关闭连接
                    close(cfd);
                    FD_CLR(cfd, &allset);
                    client[i] = -1;

                    if(cfd == maxfd){
                        while(maxfd > listenfd && !FD_ISSET(maxfd, &allset)){
                            maxfd--;
                        }   // 找到最大的那个fd
                    }
                    //更新maxi
                    if(i == maxi){
                        while(maxi > -1 && client[maxi] == -1) maxi--;
                    }

                }else if(n > 0){
                    for(int j = 0;j < n; j++)
                        if(buf[j] >= 'a' && buf[j] <= 'z')
                            buf[j] = toupper(buf[j]);
                    Writen(cfd, buf, n);
                    Writen(STDOUT_FILENO, buf, n);
                    Writen(STDOUT_FILENO, "\n", 1);
                }else if(n < 0){
                    close(cfd);
                    FD_CLR(cfd, &allset);
                    client[cfd] = -1;
                }
            }

        }
    }
    close(listenfd);
    return 0;
}