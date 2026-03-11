#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(int argc, char* argv[]) {
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(8080);

    // 设置端口可以快速重复使用 不用等待 TIME_WAIT状态
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    listen(listenfd, 5);

    printf("Waiting for connection...\n");
    
    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_addrlen);

    // 【关键修改 1】开启 OOB 在线模式
    // 这样 OOB 数据会留在普通数据流中，不会丢失，也不需要 MSG_OOB 标志来读
    int on = 1;
    setsockopt(connfd, SOL_SOCKET, SO_OOBINLINE, &on, sizeof(on));
    // setsockopt(connfd, SOL_SOCKET, SO_DEBUG, &on, sizeof(on));

    char buf[1024];
    
    while(1) {
        // 【关键逻辑】
        // sockatmark 返回 1，说明“下一个字节”就是 OOB 数据
        int res = sockatmark(connfd);
	
	    // printf("res is %d\n", res);
	    if (res == 1) {
            printf("--- At OOB Mark ---\n");
            
            // 既然是 OOB，我们单独读 1 个字节出来查看（不需要 MSG_OOB 标志）
            memset(buf, 0, sizeof(buf));
            int ret = recv(connfd, buf, 1, 0); 
            if (ret <= 0) break;
            
            printf("OOB Data: '%s'\n", buf);
        } 
        else {
            // 正常读取
            // 【重要特性】如果开启了 OOBINLINE，普通 recv 会读取数据直到遇到 OOB 标记
            // 然后它会停止！并在下一次循环时让 sockatmark 返回 true。
            memset(buf, 0, sizeof(buf));
            int ret = recv(connfd, buf, sizeof(buf)-1, 0);
            if (ret <= 0) break;
            
            printf("Normal Data: '%s' (len=%d)\n", buf, ret);
            sleep(1);
	}
    }

    close(connfd);
    close(listenfd);
    return 0;
}
