#include <stdint.h>
#include <string.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#include <stdlib.h>
#include "errno.h"
#include <stdio.h>
#include <unistd.h>      // 添加这一行 - 声明 read, write, close 函数

#include "wrap.h"
#define MAX_CLIENT  1024
#define BUF_SIZE    1024
// server: 
// 	socket	# 创建Socket
// 	bind	# 绑定地址结构
// 	listen # 设置监听上限
// 	accept # 阻塞监听客户端连接
// 	read() # 读取socket缓冲区 获取客户端数据
//  	将数据进行处理
// 	write() # 写入socket 缓冲区 发送数据
// 	close() # read读取到0，表示传输层收到了FIN报文.

struct cline_info{
    int cfd;
    struct sockaddr_in addr;
};


void* handle_client(void* arg){
    struct cline_info* client_i = (struct cline_info*)arg;
    char buf[BUF_SIZE];
    ssize_t len;
    ssize_t len_writen;
    char client_ip_str[INET_ADDRSTRLEN]; 
    
    pthread_detach(pthread_self());

    while(1){
        len = Readn(client_i->cfd, buf, BUF_SIZE);
        if(len == 0){
            // 读取到数据为0 表示链接中断 对方关闭了写缓冲区
            printf("read nothing cfd: %d\n", client_i->cfd);
            break;
        }
        for(ssize_t i = 0;i < len; i++){
            if(buf[i] >= 'a' && buf[i] <= 'z'){
                buf[i] = toupper(buf[i]);
                usleep(500000);
            }
        }
        len_writen = Writen(client_i->cfd, buf, len);
        printf("IP: %s, port: %d, Writen %ld Bytes: \n", 
            inet_ntop(AF_INET, &client_i->addr.sin_addr, client_ip_str, sizeof(client_ip_str)),
            ntohs(client_i->addr.sin_port), len_writen);
    }
    close(client_i->cfd);
    free(client_i);
    pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
    struct sockaddr_in addr;
    pthread_t tid[MAX_CLIENT];
    uint32_t i = 0;             // 位置
    char ip_str[INET_ADDRSTRLEN];    // 储存客户端IP地址方便打印输出
    struct sockaddr_in client_addr;
    struct cline_info* client_i;              // 客户端基本信息 IP cfd port;
    int ret;
    
    int lfd = Socket(AF_INET, SOCK_STREAM, 0);
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    // addr.sin_addr.s_addr = htonl(INADDR_ANY);       //绑定所有网卡
    if (argc > 1) {
        inet_pton(AF_INET, argv[1], &addr.sin_addr.s_addr);
    } else {
        addr.sin_addr.s_addr = htonl(INADDR_ANY); // 默认行为
    }

    Bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
    
    inet_ntop(AF_INET, &addr.sin_addr.s_addr, ip_str, INET_ADDRSTRLEN);
    printf("Bind IP: %s Port: %d\n", ip_str, ntohs(addr.sin_port));
    // cout << "bind IP :" << ip_str << " Port: "<< ntohs(addr.sin_port) << endl;
    Listen(lfd, 128);
    
    //accept
    while(1){
        socklen_t client_addr_len = sizeof(client_addr);
        int cfd = Accept(lfd, (struct sockaddr*)&client_addr, &client_addr_len);
        //打印相关信息
        client_i = (struct cline_info*)malloc(sizeof(struct cline_info));
        printf("Client IP: %s, Port: %d\n", 
            inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str)),
            ntohs(client_addr.sin_port));
        client_i->addr = client_addr;
        client_i->cfd = cfd;
        
        // 传递指针
        ret = pthread_create(tid + i, NULL, handle_client, (void*)client_i);
        if(ret != 0){
            fprintf(stderr, "pthread_create error: %s\n", strerror(ret));
            close(cfd);
            free(client_i);
        }
        i++;    // 自增
        if(i >= MAX_CLIENT) 
            break;
    }
    return 0;
}

