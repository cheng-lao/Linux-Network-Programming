#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        printf("usage: %s <ip_address>\n", argv[0]);
        return 1;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &server_address.sin_addr);
    server_address.sin_port = htons(8080);

    if (connect(sockfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("connect failed");
        return 1;
    }else{
    	printf("connected IP: %s, Port: %d\n", argv[1], 8080);
    }

    // 1. 发送普通数据 "123"
    const char* normal_data = "123";
    send(sockfd, normal_data, strlen(normal_data), 0);
    printf("Sent normal: 123\n");
    // 2. 发送带外数据 "A" (MSG_OOB)
    // 注意：只有最后一个字节会被认为是真正的 OOB 数据
    const char* oob_data = "A";
    send(sockfd, oob_data, strlen(oob_data), MSG_OOB);
    printf("Sent OOB: A\n");
    // 3. 发送普通数据 "456"
    const char* normal_data2 = "456";
    send(sockfd, normal_data2, strlen(normal_data2), 0);
    printf("Sent normal: 456\n");
    sleep(5);
    close(sockfd);
    return 0;
}
