#ifndef _WRAP_H_
#define _WRAP_H_

#include <cctype>
#include <cstddef>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// #include <errno.h>
#include <stdio.h>
#include <unistd.h>      // 添加这一行 - 声明 read, write, close 函数
#include <cstring>  
using namespace std;

int Socket(int domain, int type, int protocol);
void sys_err(const char* s);
int Bind(int fd, const struct sockaddr* addr, socklen_t len);
int Listen(int fd, int backlog);
int Accept(int fd, struct sockaddr* addr, socklen_t* len);
size_t Readn(int fd, void* buf, size_t nbytes);
size_t Writen(int fd, void* buf, size_t nbytes);

int Connect(int fd, const struct sockaddr *addr, socklen_t len);

#endif
