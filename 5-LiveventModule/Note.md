~~~text
libevent库： 基于“事件”的异步通信模型
实现框架(最基础的框架)： 
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
~~~






