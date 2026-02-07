
### TCP状态时序图

主动建立TCP连接： CLOSE -> 发送SYN -> SYN_SENT -> 接受ACK + SYN -> SYN_SENT -> 发送ACK -> ESTABLISHED
主动关闭TCP连接： ESTABLISHED -> 发送FIN -> FIN_WAIT1 -> 接收ACK -> FIN_WAIT2（半关闭状态） ->  
            -> 接收端发送FIN -> FIN_WAIT2 -> 发送ACK -> **TIME_WAIT(只有主动关闭TCP连接的一端才会经历这个状态)**
            -> 等待2MSL时长 -> CLOSE

被动建立连接： CLOSE -> LISTEN -> 接受SYN -> ESTABLISHED -> 发送ACK+SYN -> SYN_RCVD -> 接收ACK -> ESATBLISHED
被动关闭连接： ESTABLISHED -> 接受FIN -> ESTABLISHED -> 发送ACK -> **CLOSE_WAIT** -> 发送FIN -> LAST_ACK -> 接收ACK -> CLOSE 




#### select函数介绍


select

int select(int nfds, fd_set *readfds, fd_set *writefds,  fd_set *exceptfds, struct timeval *timeout);

nfds: 监听的所有文件描述符当中最大的文件描述符+1

（readfds, writefds, execptfds 三个传入传出参数）

readfds: 读文件描述符监听集合

writefds: 写文件描述符监听集合

exceptfds: 异常文件描述符监听集合

timeout:  > 0 设置监听超时时长

			NULL: 阻塞监听

			==0 非阻塞监听，轮询

返回值： 

		>0  所有监听集合当中，满足对应事件的总是

		=0：没有满足监听条件的文件描述符

		-1 ： errno

void FD_ZERO(fd_set* set); 清空一个文件描述符集合

void FD_SET(int fd, fd_set* set); 将待检测的描述符加入

void FD_CLR(int fd, fd_set*set) 将描述符移除出描述符集合

int FD_ISSET(int fd, fd_set* set); 判断一个文件描述符是否在监听集合当中。




