#include "../wrap.h"
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>

#define SERV_PATH "serv.socket"
// #define CLIENT_PATH "client.socket"

int main(int argc, char* argv[]){

    int sockfd;
    struct sockaddr_un addr;
    char buf[BUFSIZ];
    int nread, nwriten;
    socklen_t len = sizeof(addr);

    sockfd = Socket(AF_UNIX, SOCK_STREAM, 0);
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SERV_PATH);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path);
    // Remove the socket file if it exists
    unlink(SERV_PATH);
    Bind(sockfd, (struct sockaddr*)&addr, len);
    listen(sockfd, 128);

    while (1) {
        int client_fd = Accept(sockfd, (struct sockaddr*)&addr,&len);
        
        len -= offsetof(struct sockaddr_un, sun_path);
        addr.sun_path[len] = '\0';
        // printf("client bind filename is %s\n", addr.sun_path);
        // 客户端可以不用绑定文件，所以这里可能打印出来是空白的

        while(1){
            nread = Readn(client_fd, buf, BUFSIZ);
            if(nread == -1){
                perror("read error:");
                break;
            }
            buf[nread] = '\0';
            for (int i = 0; i < nread; i++) {
                buf[i] = toupper(buf[i]);
            }
            nwriten = Writen(client_fd, buf, nread);
            if(nwriten == -1){
                perror("write error");
                break;
            }
        }
        close(client_fd);
    }
    return 0;
}
