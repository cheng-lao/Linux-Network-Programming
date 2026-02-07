#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <event2/event.h>
#include <unistd.h>

#define PIPE_FILENAME "/tmp/fifo"

void pipe_write(int fd, short what, void *arg){
    printf("Come into %s\n", __func__);
    static int num = 0;
    char buf[1024];
    int nwriten;

    nwriten = sprintf(buf, "Pipe write num: %d", ++num);
    buf[nwriten] = '\0';
    nwriten = write(fd, buf, strlen(buf));
    if(nwriten == -1){
        perror("write fifo error");
    }
    printf("Exit away from %s\n", __func__);
    sleep(1);
    return;
}

int main(int argc, char* argv[]){
    
    int fd;
    struct event_base* base;
    struct event* ev;

    fd = open(PIPE_FILENAME, O_WRONLY | O_NONBLOCK);
    if (fd == -1) {
        perror("open fifo error");
        return 1;
    }
    //初始化 event_base，有了基座，后面的事件都可以添加到这个基座
    base = event_base_new();
    
    ev = event_new(base, fd, EV_WRITE | EV_PERSIST, pipe_write, NULL);
    event_add(ev, NULL);
    event_base_dispatch(base);

    event_free(ev);
    event_base_free(base);
    close(fd);
    return 0;
}