#include <cctype>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// #include <errno.h>
#include <stdio.h>
#include <unistd.h>      // 添加这一行 - 声明 read, write, close 函数
#include <iostream>
#include <cstring>  

#include "wrap.hpp"

using namespace std;
// server: 
// 	socket	# 创建Socket
// 	bind	# 绑定地址结构
// 	listen # 设置监听上限
// 	accept # 阻塞监听客户端连接
// 	read() # 读取socket缓冲区 获取客户端数据
//  	将数据进行处理
// 	write() # 写入socket 缓冲区 发送数据
// 	close() # read读取到0，表示传输层收到了FIN报文.



int main(int argc, char const *argv[])
{
    char buf[BUFSIZ];
    // create socket
    int fd = Socket(AF_INET, SOCK_STREAM, 0);

    // bind IP and Port, listen
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    
    // inet_pton(AF_INET, "192.168.1.100", &addr.sin_addr);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    Bind(fd, (sockaddr*)&addr, sizeof(addr));     
    
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip_str,INET_ADDRSTRLEN);
    cout << "bind IP :" << ip_str << " Port: "<< ntohs(addr.sin_port) << endl;
    
    //设置同时建立三次握手的链接数上限
    Listen(fd, 128);

    // accept 
    socklen_t size_addr = sizeof(addr);
    int conn_fd = Accept(fd, (sockaddr*)&addr, &size_addr);
    char client_ip_str[INET_ADDRSTRLEN];
    // inet_ntop(AF_INET, &addr.sin_addr, client_ip_str, INET_ADDRSTRLEN);
    cout << "Client IP: "<<
        inet_ntop(AF_INET, &addr.sin_addr, client_ip_str, INET_ADDRSTRLEN)
            <<" Port: "<< ntohs(addr.sin_port)<<endl;
    // inet_ntop 返回的字符串和调用inet_ntop函数 填充的第三个参数的返回值都是相同的
    cout << "Client IP: "<< client_ip_str <<" Port: "<< ntohs(addr.sin_port)<<endl;

    // read write
    while(1){
        int read_len = Readn(conn_fd, buf, sizeof(buf));
        if(read_len == 0){
            cerr << "read nothing!\n";
            break;
        }
        // else cout << "read_len: " << read_len << endl;
        buf[read_len] = '\0';
        cout << buf << endl; 
        for(int i = 0; i < read_len; i++){
            if(buf[i]>='a' && buf[i] <= 'z')
                buf[i] = toupper(buf[i]);
        }
        Writen(conn_fd, buf, read_len);
    }
    
    close(conn_fd);
    close(fd);

    return 0;
}

