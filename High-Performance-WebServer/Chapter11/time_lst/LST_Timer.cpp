// #include "../../../High-Performance-WebServer/Chapter11/time_lst/LST_Timer.h"
#include "LST_Timer.h"

/**
 * @brief 添加定时器`timer`到`lst_head`之后的部分链表当中
 * @param `timer` 定时器
 * @param `lst_head` 链表节点
 * @note 链表是升序链表，要将`timer`插入到`lst_head`之后最后一个比`timer`超时时间小的定时器之后
 */
void sort_timer_lst::add_timer(util_timer* timer, util_timer* lst_head){
    util_timer* prev = lst_head;
    util_timer* tmp = prev->next;
    while(tmp){
        if(tmp->expire > timer->expire){
            // 找到了超时时间比timer大的定时器，应该将timer放到tmp的前面
            prev->next = timer;
            timer->prev = prev;
            timer->next = tmp;
            tmp->prev = timer;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    // 如果没有找到合适的节点，那就直接设置成尾节点
    if( !tmp ){
        prev->next = timer;
        timer->next = nullptr;
        timer->prev = prev;
        tail = timer;
    }
}

/**
 * @brief 将目标定时器`timer`加入到链表当中
 * @param `timer`定时器
 * @note 链表是升序链表，应该将`timer`按照超时时间从小到大排序插入到合适的位置上
 */
void sort_timer_lst::add_timer(util_timer* timer){
    if(!timer) return ;
    if(!head){
        //如果此时链表为空
        head = tail = timer;
        return ;
    }
    //找到第一个超时的元素位置
    if(head->expire > timer->expire){
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    add_timer(timer, head);
}

/**
 * @brief 定时任务发生变化的时候，调整对应的定时器到链表的尾部，函数只考虑定时器超时时间延长的情况
 * @param `timer`需要调整的定时器
 */
void sort_timer_lst::adjust_timer(util_timer* timer){
    if(!timer){
        return ;
    }
    util_timer* tmp = timer->next;
    if(!tmp || (timer->expire < tmp->expire)){
        //如果定时器是最后一个或者是调整时间之后还保持递增顺序那就不用做修改
        return;
    }
    if(timer == head){
        //如果是头结点，那就将它插入到头结点之后
        head = head->next;
        timer->next = nullptr;
        head->prev = nullptr;        
        add_timer(timer, head);
    }else{
        //否则的话就先将定时器取出来，然后再放到原先位置之后
        util_timer* tmp = timer->next;
        timer->next->prev = timer->prev;    // 后继节点的前驱指针
        timer->prev->next = timer->next;    // 前驱节点的后继指针
        timer->next = nullptr;
        timer->prev = nullptr;
        add_timer(timer, tmp);
    }
}

/**
 * @brief 删除指定的定时器`timer`
 * @note 链表没有头结点和尾节点，只有首尾指针，所以要特殊考虑`timer`在头部和尾部等特殊情况
 */
void sort_timer_lst::del_timer(util_timer* timer){
    if(!timer) {
        return;
    }
    if(timer == head && head == tail){
        delete timer;
        head = tail = nullptr;
    }
    // 特殊考虑定时器在头部和尾部的情况
    if(timer == head){
        head = head->next;
        head->prev = nullptr;
        delete timer;
        return;
    }
    if(timer == tail){
        tail = tail->prev;
        tail->next = nullptr;
        delete timer;
        return;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
    return;
}

/**
 * @brief 检查时间轴，揪出所有已经到期的定时器，并触发它们的回调函数。
 * @note tick: 滴答，滴答，滴答
 */
void sort_timer_lst::tick(){
    if(!head) return;

    util_timer* tmp = head;
    time_t cur = time(NULL);
    while(tmp)
    {
        if(cur < tmp->expire){
            break;
        }
        tmp->cv_func(tmp->user_data);
        head = tmp->next;
        if(head){
            head->prev = nullptr;
        }
        delete tmp;
        tmp = head;
    }
}

