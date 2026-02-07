#include <arpa/inet.h>
#include <stdio.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

void read_cb(struct bufferevent *bev, void *ctx){
    char buf[1024];
    struct evbuffer *input;
    struct evbuffer *output;
    int nread;

    input = bufferevent_get_input(bev);
    nread = evbuffer_remove(input, buf, 1024);
    if(nread == -1){
        perror("evbuffer_remove error!");
    }else buf[nread] = '\0';
    printf("Read data: %s\n", buf);

    // 不应该在读回调函数当中来写，这样会造成循环写一直写，然后读回调函数会被一直调用
    // output = bufferevent_get_output(bev);
    // evbuffer_add(output, data, strlen(data));
    // printf("Send Message %s", data);
}

void write_cb(struct bufferevent* bev, void* ctx){
    // sleep(1);
    bufferevent_free(bev);
}

void event_cb(struct bufferevent *bev, short what, void *ctx){
    struct event_base* base = (struct event_base*) ctx;
    if(what & BEV_EVENT_CONNECTED){ // 
        printf("Connected from Sever!\n");
    }
    if(what & BEV_EVENT_ERROR){
        //遇到网络出错
        printf("bufferevent event error!\n");
        event_base_free(base);
    }
    if (what & BEV_EVENT_EOF) {
        printf("bufferevent event EOF!\n");
        event_base_loopbreak(base);
    }
}

void read_terminal(evutil_socket_t fd, short what, void* ctx){
    struct bufferevent* bev = (struct bufferevent*) ctx;
    char buf[1024];
    int nread;

    nread = read(fd, buf, sizeof(buf) - 1);
    if(nread == -1){
        return;
    }
    buf[nread] = '\0';
    
    if (nread > 0) {
        bufferevent_write(bev, buf, nread);
        printf("[Sent]: %s\n", buf);
    }
}

/*
客户端的流程:
socket -> connect 全部变为bufferevent_socket_new 和 bufferevent_socket_connect
event_base_new -> bufferevent_socket_new(创建bufferevent) -> bufferevent_socket_connect(给定客户端地址结构，连接客户端) 
-> bufferevent_setcb(设置回调函数) -> bufferevent_enable(打开读缓冲区) -> event_base_dispatch(循环监听事件)
*/
int main(int agrc, char* argv[]){

    int fd;
    struct event_base* base;
    struct sockaddr_in addr;
    struct bufferevent *bev;
    struct event* stdin_ev;

    base = event_base_new();
    // fd = socket(AF_INET, SOCK_STREAM, 0);
    
    bzero(&addr, sizeof(addr));
    // addr.sin_addr.s_addr = htonl();
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
    addr.sin_port = htons(8080);
    addr.sin_family = AF_INET;

    bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    // 其实重点还是要知道读回调函数是在读缓冲区有数据的时候 并且当前程序的主导权在libevent的时候就会主动调用都回调函数
    // 写回调函数是在写缓冲区被全部读取之后才会被调用
    // 异常回调函数是在某些特定的异常事件发生之后才会被调用
    bufferevent_socket_connect(bev, (struct sockaddr*)&addr, sizeof(addr));
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, base);
    // 开启读缓冲区 和写缓冲区（写缓冲区默认打开，这里也可以不用打开
    bufferevent_enable(bev, EV_READ | EV_WRITE);

    // 设置监听的文件描述符是标准输入文件，并且监听事件是读取事件，设置永远执行，事件发生的时候调用read_terminal函数
    // 传递bufferevent作为回调函数的参数 可以将写入的数据写入到写缓冲区当中
    stdin_ev = event_new(base, STDIN_FILENO, EV_READ|EV_PERSIST, read_terminal, bev);
    event_add(stdin_ev, NULL);
    
    // 启动循环检测
    event_base_dispatch(base);
    
    // 销毁
    event_free(stdin_ev);
    event_base_free(base);
    return 0;
}
