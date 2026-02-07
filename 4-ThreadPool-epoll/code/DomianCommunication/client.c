
#include "../wrap.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SERV_PATH "serv.socket"
#define CLIENT_PATH "client.socket"

int main(int agrc, char* argv[]){
    
    int cfd;
    struct sockaddr_un serv_addr, client_addr;
    socklen_t len;
    int nread;
    char buf[BUFSIZ];

    //1. socket创建
    cfd = Socket(AF_UNIX, SOCK_STREAM, 0);
    
    //2. 初始化服务器端的地址结构并进行 使用TCP做本地套接字的通信是可以不用绑定端口的，但是如果是用UDP做通信，就必须要进行bind绑定端口。
    bzero(&client_addr, sizeof(client_addr));
    strcpy(client_addr.sun_path, CLIENT_PATH);
    client_addr.sun_family = AF_UNIX;
    len = offsetof(struct sockaddr_un, sun_path) + strlen(client_addr.sun_path);
    unlink(CLIENT_PATH);
    Bind(cfd, (struct sockaddr*)&client_addr, len);
    
    //3. 初始化服务器端地址进行连接
    bzero(&serv_addr, sizeof(serv_addr));
    strcpy(serv_addr.sun_path, SERV_PATH);
    serv_addr.sun_family = AF_UNIX;
    len = offsetof(struct sockaddr_un, sun_path) + strlen(serv_addr.sun_path);
    
    Connect(cfd, (struct sockaddr*)&serv_addr, len);
    while (fgets(buf, BUFSIZ, stdin) != NULL) {
        Writen(cfd, buf, strlen(buf));
        nread = Readn(cfd, buf, BUFSIZ);
        if(nread == 0){
            perror("Readn error!");
            break;
        }
        Writen(STDOUT_FILENO, buf, nread);
    }
    unlink(CLIENT_PATH);
    close(cfd);
    return 0;
}

/*
没问题！这个知识点非常容易混淆，因为**网络套接字**和**本地套接字**在“自动分配地址”这方面的待遇完全不同。

以下是针对“客户端是否需要隐式绑定（`bind`）”的总结：
---

### 1. 核心结论表

| 套接字类型 | 是否需要手动 `bind`？ | 隐式绑定行为 | 后果/影响 |
| --- | --- | --- | --- |
| **网络 TCP** | **不需要** | **自动分配** IP 和临时端口 | 正常通信，最常用的做法。 |
| **网络 UDP** | **不需要** | **自动分配** IP 和临时端口 | 正常通信，服务端可以根据分配的端口回包。 |
| **本地 Stream (UDS)** | **可选** | **不会**自动创建 socket 文件 | **可以通信**，但客户端是**匿名**的。服务端通过 `accept` 拿不到客户端的文件路径。 |
| **本地 Dgram (UDS)** | **必须手动 `bind**` | **不会**自动创建 socket 文件 | 如果不手动 `bind` 路径，服务端收包后**无法回信**（因为找不到回信地址）。 |

---

### 2. 为什么本地套接字（UDS）不会自动绑定？

* **网络套接字**：内核拥有成千上万个端口（1024-65535）可以随意分配，且不需要在磁盘上创建文件，成本极低。
* **本地套接字**：地址是一个**文件路径**。如果内核要帮你“隐式绑定”，它就得在磁盘上选个地方创建一个文件。内核通常不希望未经你许可就在文件系统里乱点点（比如在哪个目录下创建？取什么名字？万一重名了怎么办？），所以它干脆不帮客户端做这件事。

---

### 3. 本地套接字 Stream 模式（你的代码）

在 `SOCK_STREAM` 下，即使客户端不 `bind`，通信依然成功的原因是：

* **连接已建立**：`connect` 动作已经在内核中建立了一对“文件描述符”的绑定关系。
* **双向通道已开**：数据直接通过内存缓冲区传递。服务端不需要知道你的“名字”（路径），只需要顺着已经建立好的连接（`client_fd`）写回去就行了。

---

### 4. 为什么你会记得“不行”？

你记忆中的“不行”，极大可能是基于 **UDP（Dgram）** 的场景。
在 UDP 下，没有建立连接的过程。服务端回包必须依赖 `sendto` 里的目标地址。如果客户端没有 `bind` 一个具体的路径，服务端拿到的 `src_addr` 就是空的。

> **就像寄信：** 如果你不写“寄信人地址”，邮局（内核）不会帮你乱编一个，对方收到信后也就没法给你回信。

---

### 5. 开发建议

* **写本地流式套接字 (TCP 型)**：建议**不 bind**。省去了处理 `client.socket` 文件冲突和 `unlink` 的麻烦，保持客户端匿名即可。
* **写本地数据报套接字 (UDP 型)**：必须 **bind**。并且每个客户端要 bind 不同的路径，否则无法接收回包。

---

这个总结理清了你之前的疑惑吗？如果你打算继续深入，我们可以聊聊**多进程并发服务器**（比如 `fork` 一个子进程处理 `client_fd`），那才是系统编程真正开始展现威力的地方！

*/