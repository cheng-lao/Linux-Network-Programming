#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define THREAD_STEP 10
#define PATIENCE_TIME 10
#define RATIO 1.4
// #define NORMAL_REDUCTION_METHOD 

typedef struct{
	void (*function)(void*);
	void* arg;
} threadpool_task_t;

typedef struct {
	pthread_mutex_t lock;				// 用于锁住本结构体
	pthread_mutex_t thread_counter;		// 记录忙状态线程的个数的锁

	pthread_cond_t queue_not_full;		// 当任务队列满是，添加任务的额线程阻塞，等待此条件变量
	pthread_cond_t queue_not_empty;		// 任务队列不为空时， 通知等待任务的线程

	pthread_t* threads;					// 存放线程池当中每个线程的tid，数组
	pthread_t manager_tid;				// 存储管理线程ID
	threadpool_task_t *task_queue;		// 任务队列（数组首地址）

	int min_thr_num;					// 线程池最小线程数
	int max_thr_num;					// 线程池最大的线程数
	int live_thr_num;					// 当前存活的线程个数
	int busy_thr_num;					// 忙线程的个数
	int wait_exit_thr_num;				// 要销毁的线程个数

	int queue_front;					// 队头下标
	int queue_rear;						// 队尾下标
	int queue_size;						// task_queue队列中实际任务数
	int queue_capacity;					// task_queue队列可容纳任务数上限

	int shutdown;						// 线程池使用状态
}threadpool_t;


// typedef struct ThreadPool ThreadPool;
// 创建线程池并初始化
threadpool_t *threadpool_create(int min, int max, int queueSize);

// 销毁线程池
int threadpool_destroy(threadpool_t* pool);

// 给线程池添加任务
void threadpool_add(threadpool_t* pool, void(*func)(void*), void* arg);

// 获取线程池中工作的线程的个数
int threadpool_busynum(threadpool_t* pool);

// 获取线程池中活着的线程的个数
int threadpool_alivenum(threadpool_t* pool);

// 工作的线程(消费者线程)任务函数
void* worker(void* arg);
// 管理者线程任务函数
void* manager(void* arg);
// 单个线程退出
void thread_exit(threadpool_t* pool);


#endif  // _THREADPOOL_H