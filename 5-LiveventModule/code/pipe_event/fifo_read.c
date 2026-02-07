
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <event2/event.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>

#define PIPE_FILENAME "/tmp/fifo"

void read_pipe(int fd, short what, void* arg){
    
    // printf("Come into %s\n", __func__);
    struct event_base* base = (struct event_base*)arg;
    char buf[1024];
    int nread;

    nread = read(fd, buf, sizeof(buf));
    if(nread == -1){
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            return ;
        }
    }else if(nread == 0){
        event_base_loopbreak(base);
    }
    printf("read data: %s\n", buf);
    // printf("Exit away from %s\n", __func__);
    usleep(500000);
}

//一个基础的 event使用框架
/*
1,创建event_base
	struct event_base* base = event_base_new();

2,创建事件event
	事件分为两种
		常规事件(event) ---> event_new()
		buffer事件(bufferevent) ----> bufferevent_socket_new()

3, 将事件添加到 base上
	int event_add(struct event* ev,const struct timeval *val)
    
4, 循环监听事件满足
    int event_base_dispatch(struct event_base* base);

5, 释放event_base
    event_base_free(base);
*/

int main(int argc, char* agrv[]){

    int fd;
    struct event_base* base;
    struct event* ev;

    unlink(PIPE_FILENAME);
    mkfifo(PIPE_FILENAME, 0664);
    // 这里设置O_NONBLOCK关键字就是为了不让open函数阻塞，
    // 一般来说读写两端，先打开的一段会等待另一端打开相同的pipe文件之后能执行后续的代码
    fd = open(PIPE_FILENAME, O_RDONLY | O_NONBLOCK);

    if(fd == -1){
        perror("open error!");
        exit(1);
    }

    //创建一个 event_base
    base = event_base_new();
    
    // 这里的what参数是EV_READ，所以等待的事件是fd变为可读的时候
    // 无 EV_PERSIST	触发一次后事件被移除，需要再次 event_add 才能继续监听
    // 有 EV_PERSIST    触发后事件继续保留，可多次触发	                    
    ev = event_new(base, fd, EV_READ | EV_PERSIST, read_pipe, base);
    // 添加事件，第二个参数是超时事件，如果传递NULL表示不设定超时事件
    // 如果传递非NULL参数，在倒计时结束之后会直接调用回调函数。
    event_add(ev, NULL);
    event_base_dispatch(base);
/*
event_base_dispatch() 的作用是：
进入事件循环，不断检查已注册的 fd 是否有可读/可写等事件 (可以认为是while循环，会发生阻塞)
有事件发生时，调用对应回调（如 read_pipe）

event_base_dispatch() 什么时候会返回？
循环会在下面几种情况下退出并返回：
情况	说明
1. 没有待处理事件	event_base 里没有已注册且未完成的事件，循环自然结束
2. 调用 event_base_loopbreak(base)	立即退出循环，类似 break
3. 调用 event_base_loopexit(base, NULL)	在当前事件处理完后退出，类似“优雅退出”
4. 发生错误
*/
    //释放资源
    event_free(ev);
    event_base_free(base);
    close(fd);

    return 0;
}
/*
我不太理解为什么fd 仍然“可读”，管道中如果有数据不会直接拿掉嘛？然后下次因为管道没数据并且对端关闭所以就直接返回0 了?之后一直都没有读事件发生?
核心在于：**epoll/select 里的“可读”和我们直觉里的“有数据”不是一回事**。

## 什么是“可读”？

对 epoll/select 来说，fd 可读的意思是：

> 此时调用 `read()` **不会阻塞**，能立刻返回。

并不要求“管道里还有数据”。

## 具体过程

1. **写端关闭前**：管道里有数据  
   - `read()` 读到数据，正常返回 n > 0，数据被消费  
   - 管道变空

2. **写端已关闭，管道已空**：  
   - 再调用 `read()`：会**立刻**返回 0（EOF），不会阻塞  
   - 所以对 epoll 来说：这个 fd 是“可读”的  
   - 因此会持续触发 `EV_READ`

3. **为什么会一直触发？**  
   - 每次触发 → 回调里 `read()` → 返回 0  
   - 由于 `EV_PERSIST`，事件仍注册  
   - fd 仍然满足“read() 不会阻塞” → 下一轮循环继续触发  
   - 如此循环，形成“EOF 风暴”

## 关键区别

| 概念 | 含义 |
|------|------|
| 直觉里的“可读” | 管道里有数据，`read()` 能读到字节 |
| epoll/select 的“可读” | `read()` 不会阻塞，能马上返回（可以是数据、0 或错误） |

写端关闭且管道空时，`read()` 会立刻返回 0，因此 fd 一直被视作“可读”。

## 对比：如果是阻塞的 fd

假设用阻塞模式、没有 epoll：

- `read()` 会阻塞等待数据  
- 写端关闭后，`read()` 会返回 0 并退出，不会一直卡住

但在非阻塞 + epoll 的模式下：每次返回 0 后，epoll 仍认为 fd 可读，所以会不断通知你，导致回调一直执行。

**结论**：要避免这种循环，必须在回调里检查 `read() == 0`，发现 EOF 后移除事件或退出事件循环，否则 fd 会一直被当成可读。
*/

