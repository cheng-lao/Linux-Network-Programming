#include<arpa/inet.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>

#define SERVER_IP "101.132.189.102"
#define SERVER_PORT 8080

int main(int argc, char const *argv[])
{
    int fd;
    struct sockaddr_in addr;
    char ip_str[INET_ADDRSTRLEN];
    char buf[BUFSIZ];
    int ret, len, nread;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr.s_addr);
    
    ret = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if(ret == -1){
        perror("connect error");
        return 1;
    }
    printf("Making connection with IP: %s Port: %d\n", SERVER_IP, SERVER_PORT);
    
    while(fgets(buf, BUFSIZ - 1, stdin) != NULL){
        write(fd, buf, strlen(buf));
        nread = read(fd, buf, BUFSIZ - 1);
        // 如果客户端没有下面这些读取的程序只有一行 write(STDOUT_FILENO, buf, strlen(buf))服务器程序就会崩溃 why????
        if(nread <= 0){
            if(nread == 0){
                printf("Server closed Connection!\n");
            }else{
                perror("read error!");
            }
            break;
        }
        write(STDOUT_FILENO, buf, strlen(buf));
    }
    close(fd);
    return 0;
}
