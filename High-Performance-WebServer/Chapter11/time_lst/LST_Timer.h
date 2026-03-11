#ifndef _LST_TIMER_HPP_
#define _LST_TIMER_HPP_
#include<time.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<stdlib.h>
using namespace std;
#define BUFFER_SIZE 128

struct client_data{
    sockaddr_in address;        // 客户端地址结构
    int sockfd;                 // socket flie descriptor
    char buf[BUFFER_SIZE];      // buf
    util_timer* timer;          // 定时器
};


// 定时器链表节点
class util_timer
{
public:
    util_timer(): prev (NULL), next(NULL){}
public:
    time_t expire;              // 绝对时间
    void (*cv_func) (client_data*);
    // 函数指针
    struct client_data* user_data;
private:
    friend class sort_timer_lst;
    util_timer* prev;
    util_timer* next;
};


class sort_timer_lst
{
private:
    /* data */
    util_timer* head;
    util_timer* tail;
    void add_timer(util_timer* , util_timer* );

    ~sort_timer_lst(){
        util_timer* tmp = head;
        // 删除所有节点
        while(tmp){
            head = tmp->next;
            delete tmp;
            tmp = head;
        }
    }
public:
    sort_timer_lst(): head(nullptr), tail(nullptr){}
    
    void add_timer(util_timer* timer);
    void adjust_timer(util_timer* timer);
    void del_timer(util_timer* timer);
    void tick();
};


#endif