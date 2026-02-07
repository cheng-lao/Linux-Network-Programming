#ifndef _WRAP_H_
#define _WRAP_H_

#include <stddef.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <stdio.h>
#include <unistd.h>      // 添加这一行 - 声明 read, write, close 函数
#include <string.h>  

int Socket(int domain, int type, int protocol);
void sys_err(const char* s);
void sys_err_child(const char* s);
int Bind(int fd, const struct sockaddr* addr, socklen_t len);
int Listen(int fd, int backlog);
int Accept(int fd, struct sockaddr* addr, socklen_t* len);
ssize_t Readn(int fd, void* buf, size_t nbytes);
ssize_t Writen(int fd, void* buf, size_t nbytes);

int Connect(int fd, const struct sockaddr *addr, socklen_t len);

#endif
