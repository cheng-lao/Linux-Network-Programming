#include <asm-generic/errno.h>
#include <stddef.h>
#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "errno.h"

void sys_err(const char* s){
    printf("%s\n", s);
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
        printf("bind failure: %s\n", strerror(errno));  // errno is a global variable
        exit(1);
    }
    return res;
}

int Listen(int fd, int backlog){
    int res = listen(fd, backlog);
    if(res == -1){
        printf("set listen_count failure: %s\n", strerror(errno)); 
        exit(1);
    }
    return res;
}

int Accept(int fd, struct sockaddr *addr, socklen_t *len){
    int res;
again:
    res = accept(fd, addr, len);
    if(res == -1){
        if(errno == EINTR){
            goto again;
        }
        printf("accept failure: %s\n", strerror(errno)); 
    }
    return res;
}

// 这个函数会使得阻塞模式下只能读取一次就返回，并不能保证读取缓冲区的所有内容，也不能保证读满缓冲区
// 如果缓冲区的所有内容太大，则只能截取一部分读取走
// 对于非阻塞文件描述符有可能会造成死循环，不建议在非阻塞模式（epollET）下使用这个函数
ssize_t Readn(int fd, void *buf, size_t nbytes){
    int len;
again:
    len =  read(fd, buf, nbytes);
    if(len == -1){
        if(errno == EINTR ){
            goto again;
        }else if(errno == EAGAIN || errno == EWOULDBLOCK){
            return len; // 非阻塞模式会遇到这些
        }
        printf("read buf failure: %s\n", strerror(errno));
        return -1;   // _exit会结束整个进程
    }
    return len; 
}
//在这里附一个C标准库的Readn实现思路
/*
ssize_t readn(int fd, void *vptr, size_t n) {
    size_t  nleft;    // 还需要读多少
    ssize_t nread;    // 本次读到了多少
    char   *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                nread = 0;      // 信号中断，再次调用 read
            else
                return -1;      // 真正出错
        } else if (nread == 0)
            break;              // EOF（对端关闭），提前退出

        nleft -= nread;
        ptr   += nread;
    }
    return (n - nleft);         // 返回实际读取的字节数
}
*/



ssize_t Writen(int fd, void* buf, size_t nbytes){
    ssize_t ret, nwritten = 0;
    char* ptr = (char*) buf;

    while(nbytes != 0 ){
        // (ret = write(fd, buf + nr, nbytes)) != 0
        ret = write(fd, ptr, nbytes);
        if(ret == -1){
            if(errno == EINTR){
                continue;
            }
            // 真正的错误发生（例如 EPIPE, ECONNRESET 等）
            // 此时应该返回 -1，告诉调用者出大事了
            perror("Write error");
            return -1;
        }
        nwritten += ret;
        nbytes -= ret;
        ptr += ret;
    }
    return (ssize_t)nwritten;
}

int Connect(int fd, const struct sockaddr *addr, socklen_t len){
    int res;
again:
    res = connect(fd, addr, len);   // addr is server addr_IP and addr_port
    if(res == -1){
        if(errno == EINTR){
            goto again;
        }
        printf("connect failure: %s\n", strerror(errno));
        exit(1);
    }
    return res;
}
