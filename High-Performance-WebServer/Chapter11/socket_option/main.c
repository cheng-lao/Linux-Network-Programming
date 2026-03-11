#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<assert.h>
#include<stdio.h>
#include<errno.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include <libgen.h> // POSIX version
#include<sys/time.h>

// SO_RCVTIMEO 选项设置socket接受数据超时时间
// SO_SNDTIMEO 选项设置socket发送数据超时时间
// 这两个选项仅对socket系统API有作用，send sendmsg recv recvmsg accept connect 
// 当前文件写的是客户端代码 socket->setsockopt->connect->write->send->close

int timeout_connect(const char* ip, int port, int time){
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr.s_addr);
    
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);   //创建一个 link socket fd
    assert(sockfd > 0);

    struct timeval timeout;
    timeout.tv_sec = time;
    timeout.tv_usec = 0;
    socklen_t len = sizeof(timeout);
    ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, len);
    assert(ret != -1);

    ret = connect(sockfd, (struct sockaddr*)&address, sizeof(address));
    if(ret == -1){
        if(ret == EINPROGRESS){
            //处理超时情况下的任务
            printf("connecting timeout!, process timeout logic \n");
            return -1;
        }
        printf("Error, occuring some problems!\n");
        return -1;
    }
    return sockfd;
}


int main(int argc,char* argv[]){
    
    if (argc <= 2)
    {
        printf("usgae: %s ip_address port_number \n", basename(argv[0]));
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int sockfd = timeout_connect(ip, port, 10);
    if(sockfd < 0) return 1;
    return 0;
}
