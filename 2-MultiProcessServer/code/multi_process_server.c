#include <sys/wait.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "errno.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>      // 添加这一行 - 声明 read, write, close 函数

#include "wrap.h"

void handle_process(int cfd){
    char buf[BUFSIZ];
    while(1){
        int read_len = Readn(cfd, buf, sizeof(buf));
        if(read_len == 0){
            printf("Client closed connection");
            //perror() 用于打印错误，格式为："前缀: 错误描述\n"
            //所以使用perror 会输出errno的值代表的含义
            close(cfd);
            _exit(0);
        }
        if(read_len < 0){
            perror("read error");
            close(cfd);
            _exit(1);
        }

        buf[read_len] = '\0';
        printf("%s", buf);
        for(int i = 0; i < read_len; i++){
            if(buf[i]>='a' && buf[i] <= 'z')
                buf[i] = toupper(buf[i]);
        }
        Writen(cfd, buf, read_len);
    }
}

void recycle_child(int childno){
    (void)childno;       // 表示这个参数用不到 
    int pid, status;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        printf("return status: %d\n", WEXITSTATUS(status));   
    }
    if (pid == -1 && errno != ECHILD) {
        sys_err("waitpid error\n");
    }

}

int main(int argc, char const *argv[])
{
    char client_ip_str[INET_ADDRSTRLEN];
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    // create socket
    int lfd = Socket(AF_INET, SOCK_STREAM, 0);

    // bind IP and Port, listen
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);

    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    Bind(lfd, (struct sockaddr*)&addr, sizeof(addr));     
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip_str,INET_ADDRSTRLEN);
    printf("bind IP :%s Port: %d\n", ip_str, ntohs(addr.sin_port));
    
    //设置同时建立三次握手的链接数上限
    Listen(lfd, 128);

    //设计捕捉函数 回收子进程
    //main process
    struct sigaction act;
    act.sa_handler = recycle_child;
    //sa_mask 是一个信号集（Signal Set）。它的作用是：定义在信号处理函数（handler）执行期间，应该被阻塞（Block）的信号列表。
    // 如果想要屏蔽一些信号（比如 SIGINT 和 SIGQUIT）可以这么写
    // sigaddset(&act.sa_mask, SIGINT);   // 对应 Ctrl+C
    // sigaddset(&act.sa_mask, SIGQUIT);  // 对应 Ctrl+反斜杠
    sigemptyset(&act.sa_mask);  // 清空信号掩码
    act.sa_flags = 0;       // 设置默认标志位
    
    if (sigaction(SIGCHLD, &act, NULL) == -1) {
        perror("sigaction error");  //注册信号量
        exit(1);
    }

    //accept
    client_addr_len = sizeof(client_addr);
    while(1){
        int cfd = Accept(lfd, (struct sockaddr*)&client_addr, &client_addr_len);
        //check cfd;
        if(cfd == -1) continue; // cfd==-1
        //print client ip and port;
        printf("Client IP:%s Port %d\n", 
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip_str, INET_ADDRSTRLEN),
            ntohs(client_addr.sin_port)
        );
        pid_t pid = fork();
        if(pid == 0){
            // child process
            close(lfd);      // close link socket
            handle_process(cfd);  // 
            _exit(0);    // WEXITSTATUS(status)可以取得数字
        }else if(pid > 0){      
            // father process
            close(cfd);     // close child socket 因为父进程用不到cfd，但是进程之间是共享这个的，所以需要关闭
            continue;           // accept continue  循环继续接收客户端发来的链接
        }else{
            sys_err("fork error\n");
            break;
        }
    }

    return 0;
}

