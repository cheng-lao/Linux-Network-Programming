#include <asm-generic/errno.h>
#include <stddef.h>
#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "errno.h"

void sys_err(const char* s){
    printf("%s\n", s);
    exit(1);
}

void sys_err_child(const char* s){
    printf("%s\n", s);
    _exit(1);
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

ssize_t Readn(int fd, void *buf, size_t nbytes){
    int len;
again:
    len =  read(fd, buf, nbytes);
    if(len == -1){
        if(errno == EINTR ){
            goto again;
        }else if(errno == EAGAIN || errno == EWOULDBLOCK){

        }
        printf("read buf failure: %s\n", strerror(errno));
        return -1;   // _exit会结束整个进程
    }
    return len; 
}
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
