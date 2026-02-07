#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/buffer.h>

#define PORT 8080

/**
 * 读回调：当由于 socket 收到数据，Libevent 帮我们读到了 Input Buffer 中时触发
 */
void read_cb(struct bufferevent *bev, void *ctx)
{
    char buf[1024] = {0};
    size_t len;

    // 1. 直接使用 bufferevent_read，这就跟你理解的普通 read 一样简单
    // 它会自动把数据从 Input Buffer 拿出来放到 buf 里
    len = bufferevent_read(bev, buf, sizeof(buf) - 1);

    if (len > 0) {
        printf("[Read Callback] 收到客户端数据: %s\n", buf);

        // 构造回复数据
        char reply[1024];
        snprintf(reply, sizeof(reply), "Server received: %s", buf);

        // 2. 发送数据：注意，这里只是把数据“塞进”了 bev 的 Output Buffer
        // 并不是立刻发送到网络。Libevent 会自动找机会发。
        bufferevent_write(bev, reply, strlen(reply));

        
        printf("[Read Callback] 数据已塞入输出缓冲区，等待内核发送...\n");
    }
}

/**
 * 写回调：当 Output Buffer 中的数据被内核“抽干”了（发送完了）的时候触发
 * 也就是水位线到达 低水位（默认是0）时触发
 */
void write_cb(struct bufferevent *bev, void *ctx)
{
    // 获取输出缓冲区的当前大小（此时应该是0，或者很小）
    struct evbuffer *output = bufferevent_get_output(bev);
    size_t remain = evbuffer_get_length(output);

    printf("[Write Callback] 触发！Output Buffer 数据已被搬运到内核。剩余滞留数据量: %zu\n", remain);
    printf("--------------------------------------------------\n");
}

/**
 * 事件回调：处理连接断开、错误等
 */
void event_cb(struct bufferevent *bev, short events, void *ctx)
{
    if (events & BEV_EVENT_EOF) {
        printf("Connection closed.\n");
    } else if (events & BEV_EVENT_ERROR) {
        printf("Error: %s\n", strerror(errno));
    }

    // 释放 bufferevent，这会自动关闭底层的 socket fd
    bufferevent_free(bev);
}

/**
 * 监听器回调：当有新连接 accept 成功时触发
 *  listener: 监听器
 *  fd: 文件描述符
 *  addr: 客户端地址接口
 *  socklen: 客户端地址长度
 *  ctx: 上下文环境，指针参数
 */
void accept_conn_cb(struct evconnlistener *listener,
                    evutil_socket_t fd, struct sockaddr *addr, int socklen,
                    void *ctx)
{
    struct event_base *base = evconnlistener_get_base(listener);
    
    // 1. 为这个新连接创建一个 bufferevent
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        fprintf(stderr, "Error constructing bufferevent!\n");
        event_base_loopbreak(base);
        return;
    }

    // 2. 设置回调函数：读、写、事件
    // 参数顺序：bev, read_cb, write_cb, event_cb, 参数
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);

    // 3. 启用读写事件（如果不 enable，回调永远不会被调用）
    // EV_PERSIST 在 bufferevent 中是默认行为，不需要显式写
    bufferevent_enable(bev, EV_READ | EV_WRITE);

    printf("New connection accepted! Waiting for data...\n");
}

int main(int argc, char **argv)
{
    struct event_base *base;
    struct evconnlistener *listener;
    struct sockaddr_in sin;

    //1. 创建一个base_new
    base = event_base_new();
    if (!base) {
        fprintf(stderr, "Could not initialize libevent!\n");
        return 1;
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);

    //2.创建监听器 设置回调函数（重点）
    // evconnlistener_new_bind这个是一个完成了socket, bind, listen, accept四个函数的集成函数
    listener = evconnlistener_new_bind(base, accept_conn_cb, NULL,
        LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
        (struct sockaddr*)&sin, sizeof(sin));

    if (!listener) {
        fprintf(stderr, "Could not create a listener!\n");
        return 1;
    }

    printf("Server started on port %d. Use 'nc 127.0.0.1 %d' to test.\n", PORT, PORT);

    // 进入事件主循环
    event_base_dispatch(base);

    evconnlistener_free(listener);
    event_base_free(base);
    return 0;
}