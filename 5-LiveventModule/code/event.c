#include <event2/event.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char* argv[]){

    const char ** buf;
    const char *buf_str;
    struct event_base* base = event_base_new();
    buf = event_get_supported_methods();
    printf("Starting Libevent %s.  Available methods are:\n", event_get_version());
    for (int i = 0; buf[i] != NULL; i++) {
        printf("buf[%d] = %s\n", i, buf[i]);
    }
    printf("--------------------------------\n");
    buf_str = event_base_get_method(base);
    printf("buf_str is %s\n", buf_str);
    // 事件销毁
    event_base_free(base);

    return 0;
}   