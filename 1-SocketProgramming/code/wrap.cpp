#include "wrap.hpp"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

void sys_err(const char* s){
    cerr << s << endl;;
    exit(1);
}
int Socket(int domain, int type, int protocol){
    int n;
    n = socket(domain, type, protocol);
    if(n == -1)
        sys_err("socket create failure!");
    return n;  // return socket descriptor
}

int Bind(int fd, const struct sockaddr *addr, socklen_t len){
    int res = bind(fd, addr, len);
    if(res == -1){
        cerr << "bind failure: " << strerror(errno)<< endl;  
        exit(1);
    }
    return res;
}

int Listen(int fd, int backlog){
    int res = listen(fd, backlog);
    if(res == -1){
        cerr << "set listen_count failure: " << strerror(errno)<< endl; 
    }
    return res;
}

int Accept(int fd, struct sockaddr *addr, socklen_t *len){
    int res = accept(fd, addr, len);
    if(res == -1){
        cerr << "accept failure: " << strerror(errno)<< endl; 
    }
    return res;
}

size_t Readn(int fd, void *buf, size_t nbytes){
    int len = read(fd, buf, nbytes);
    if (len == -1) {
        // 检查是不是因为“没数据”导致的伪错误
        // 注意：这里用 || 连接，同时检查两个常量
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 这不是真正的错误！
            // 只是说明现在缓冲区是空的，你可以过一会儿再来读，或者去干点别的事。
            printf("暂时没数据，稍后再试...\n");
        } else {
            // 这才是真正的读错误（比如连接断了，权限不够等）
            perror("read error");
        }
    }
    return len; 
}
size_t Writen(int fd, void* buf, size_t nbytes){
    int len = write(fd, buf, nbytes);
    if(len == -1){
        cerr << "write buf failure: " << strerror(errno)<< endl;
        exit(1);
    }
    return len;
}

int Connect(int fd, const struct sockaddr *addr, socklen_t len){
    int res = connect(fd, addr, len);   // addr is server addr_IP and addr_port
again:
    if(len == -1){
        if(errno == EINTR){
            goto again;
        }
        cerr << "connect failure: " << strerror(errno)<< endl;
        exit(1);
    }
    return res;
}