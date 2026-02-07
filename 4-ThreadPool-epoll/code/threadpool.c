#include "threadpool.h"
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// 创建线程池内容
threadpool_t* threadpool_create(int min, int max, int queueSize){
	threadpool_t* pool = (threadpool_t*) malloc(sizeof(threadpool_t));
	do {
		if(pool == NULL){
			printf("malloc threadpool fail...\n");
			break;
		}
		pool->threads = (pthread_t*) malloc(sizeof(pthread_t)*max);
		if(pool->threads == NULL){
			printf("malloc threads ids fail...\n");
			break;
		}
		memset(pool->threads, 0, sizeof(pthread_t) * max);
		
		// 初始化线程池相关变量
		pool->max_thr_num = max;
		pool->min_thr_num = min;
		pool->busy_thr_num = 0;
		pool->live_thr_num = 0;				// 线程存活但是不一定在忙，可能是阻塞等待任务
		pool->wait_exit_thr_num = 0;		// 线程还有等待被销毁处理的

		// 初始化锁和条件变量
		if((pthread_mutex_init(&pool->lock, NULL) != 0) ||
		(pthread_mutex_init(&pool->thread_counter, NULL)!= 0)||
		(pthread_cond_init(&pool->queue_not_empty, NULL) != 0) ||
		(pthread_cond_init(&pool->queue_not_full, NULL) != 0)
	){
		printf("mutex or condition init fail...\n");
		break;
		}

		// 初始化任务队列相关的变量
		pool->task_queue = (threadpool_task_t*)malloc(sizeof(threadpool_task_t)*queueSize);
		pool->queue_capacity = queueSize;
		pool->queue_size = 0;
		pool->queue_front = 0;
		pool->queue_rear = 0;

		pool->shutdown = 0;
		if(pthread_create(&pool->manager_tid, NULL, manager, pool) != 0){
			perror("pthread_create manager eror:");
			break;
		}
		for (int i = 0; i < min; i++) {
			if(pthread_create(&pool->threads[i], NULL, worker, pool) != 0){
				fprintf(stderr, "pthread_create worker %d error: %s\n", i, strerror(errno));
				break;
			}
			pool->live_thr_num++;	//每创建一个就++
		}
		return pool;
	}while (0);
	//中间有过程创建失败
	if(pool && pool->threads) free(pool->threads);
	if(pool && pool->task_queue) free(pool->task_queue);
	if(pool) free(pool);
	return NULL;
}

int threadpool_destroy(threadpool_t*  pool){
	if(pool == NULL){
		return -1;
	}
    // 关闭线程池
	pool->shutdown = 1;
    // 阻塞回收管理者线程
	pthread_join(pool->manager_tid, NULL);
    // 唤醒阻塞的消费者线程
	for(int i = 0;i < pool->live_thr_num; i++){ // 为什么这里要这么做??
        // 这么做会唤醒worker当中因为 pool->queue_not_empty的线程
        // 唤醒的线程因为pool->shutdown为1 会主动退出线程
		pthread_cond_signal(&pool->queue_not_empty);
	}
	// 释放内存
	if(pool->task_queue){
		free(pool->task_queue);
	}
	if(pool->threads){
		free(pool->threads);
	}
	// 销毁
	pthread_mutex_destroy(&pool->lock);
	pthread_mutex_destroy(&pool->thread_counter);
	pthread_cond_destroy(&pool->queue_not_empty);
	pthread_cond_destroy(&pool->queue_not_full);

	free(pool);
	pool = NULL;
	return 0;
}

// 向线程池队列当中添加任务，注意不是创建线程!
void threadpool_add(threadpool_t* pool, void (*func)(void*), void* arg){
    // 互斥读写线程池内容
    pthread_mutex_lock(&pool->lock);
    while (pool->queue_size == pool->queue_capacity && !pool->shutdown) {
        // 阻塞生产者线程 因为现在任务队列满了，所以没办法再添加任务了
        pthread_cond_wait(&pool->queue_not_full, &pool->lock);
    }
    if(pool->shutdown){
        // 为什么这里还要再判断是否shutdown? 能出while循环不就说明了一定是没有shutdown的?
        // 如果上面的循环退出是因为pool->queue_size < pool->queue_capactity的话，那还需要再判断一下pool->shutdown
        pthread_mutex_unlock(&pool->lock);
        return ;
    }
    // 在任务队列当中添加任务
    pool->task_queue[pool->queue_rear].function = func;
    pool->task_queue[pool->queue_rear].arg = arg;
    pool->queue_rear = (pool->queue_rear + 1) % pool->queue_capacity;
    pool->queue_size++;

    // 添加任务之后 唤醒等待条件的线程
    pthread_cond_signal(&pool->queue_not_empty);
    // 解锁
    pthread_mutex_unlock(&pool->lock);
}

int threadpool_busynum(threadpool_t *pool){
    // 为什么要专门设计这么一个锁pool->thread_counter?
    // 回答: 为了减小锁的粒度，提高并发性能。
    pthread_mutex_lock(&pool->thread_counter);
    int busynum = pool->busy_thr_num;
    pthread_mutex_unlock(&pool->thread_counter);
    return busynum;
}

int threadpool_alive_num(threadpool_t* pool){
    pthread_mutex_lock(&pool->lock);
    int alivenum = pool->live_thr_num;
    pthread_mutex_unlock(&pool->lock);
    // 加锁，取数，再解锁返回，可是这样的话如果读出来的变量在之后的几个时钟周期内刚还改变了不就是过时的数据了吗?
    // 上述问题的确存在，但是在工程当中这种数据的'snapshot'快照是可以接受的，其他地方也是同理的
    // 如果在使用这个变量的过程当中，一直加锁，会造成很严重的性能损失
    return alivenum;
}

void* worker(void* arg){
    threadpool_t* pool = (threadpool_t*) arg;
    int ret;
    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_nsec += PATIENCE_TIME;
    while (1) {
        pthread_mutex_lock(&pool->lock);

        while (pool->queue_size == 0 && !pool->shutdown) {
            // 等待条件变量的获取，任务队列当中必须要有可以工作的任务才能继续往下走
            ret = pthread_cond_timedwait(&pool->queue_not_empty, &pool->lock, &abstime);
            // wait函数会不中断依次执行 阻塞 -> 释放锁 -> 等待到条件成立 -> 重新获取锁返回 
#ifdef NORMAL_REDUCTION_METHOD
            // 当重新获取锁之后，判断是否需要销毁线程，在等待条件成立的这段时间里可能会有需要销毁的线程
            if(pool->wait_exit_thr_num > 0){
                pool->wait_exit_thr_num--;
                if(pool->live_thr_num > pool->min_thr_num){
                    pool->live_thr_num--;
                    pthread_mutex_unlock(&pool->lock);
                    thread_exit(pool);  // 会调用pthread_exit 结束线程
                }
            }
#else
            // 线程自己通过是否超长时间来 控制是否要退出
            if (ret == ETIMEDOUT) {
                // 超时了，说明10秒都没活干
                if (pool->live_thr_num > (int)(pool->min_thr_num * RATIO)) {
                    pool->live_thr_num--;
                    pthread_mutex_unlock(&pool->lock);
                    thread_exit(pool); // 自杀
                }
            }
#endif
        }
        
        if(pool->shutdown){
            pthread_mutex_unlock(&pool->lock);
            thread_exit(pool);
        }
        
        // 从任务队列当中取出一个任务
        threadpool_task_t task;
        task.function = pool->task_queue[pool->queue_front].function;
        task.arg = pool->task_queue[pool->queue_front].arg;
        // 移动头节点
        pool->queue_front = (pool->queue_front + 1) % pool->queue_capacity;
        pool->queue_size--;
        //解锁
        pthread_cond_signal(&pool->queue_not_full);
        pthread_mutex_unlock(&pool->lock);
        
        printf("thread %ld start working ...\n", pthread_self());
        pthread_mutex_lock(&pool->thread_counter);
        pool->busy_thr_num++;
        pthread_mutex_unlock(&pool->thread_counter);
        // 执行处理函数
        task.function(task.arg);

        // 回收资源
        // free(task.arg);
        // task.arg = NULL;

        printf("thread %ld end working...\n", pthread_self());
        pthread_mutex_lock(&pool->thread_counter);
        pool->busy_thr_num--;
        pthread_mutex_unlock(&pool->thread_counter);
    }
    return NULL;
}   

void* manager(void* arg){
    threadpool_t* pool = (threadpool_t*) arg;
    int patience_time = 0;
    while (!pool->shutdown) {
        //每隔1秒检测一次
        sleep(1);
        // if(patience_time == PATIENCE_TIME){
        //     patience_time = 0;
        // }

        pthread_mutex_lock(&pool->lock);
        int queue_size = pool->queue_size;
        int live_num = pool->live_thr_num;
        pthread_mutex_unlock(&pool->lock);

        // 取出忙的线程的数量
        pthread_mutex_lock(&pool->thread_counter);
        int busy_num = pool->busy_thr_num;
        pthread_mutex_unlock(&pool->thread_counter);
        
        //添加线程
        if(queue_size > live_num && live_num < pool->max_thr_num){
            pthread_mutex_lock(&pool->lock);
            int counter = 0;
            // NUMBER应该为多少 ?
            for (int i = 0; i < pool->max_thr_num  && counter < THREAD_STEP
                && pool->live_thr_num < pool->max_thr_num ; i++) {
                if(pool->threads[i] == 0){
                    pthread_create(&pool->threads[i], NULL, worker, pool);
                    counter++;
                    pool->live_thr_num++;
                }
            }
            pthread_mutex_unlock(&pool->lock);
        }
#ifdef  NORMAL_REDUCTION_METHOD
        // pool->min_thr_num是只读变量可以直接读取
        if((busy_num * 2 < live_num && patience_time++ < PATIENCE_TIME) // 如果忙线程太少了，并且这个状况持续了PATIENCE_TIME秒 就来清理多余的线程
        && live_num > pool->min_thr_num){
            patience_time = 0;  // 值置为0 表示现在的

            pthread_mutex_lock(&pool->lock);
            // pool->wait_exit_thr_num = NUMBER;
            // 实际忙碌的线程数量可能是小于规定的最小线程数量的,所以这里做取最大值
            pool->wait_exit_thr_num = (live_num - busy_num) / 2 ;
            if(pool->live_thr_num - pool->wait_exit_thr_num < pool->min_thr_num){
                pool->wait_exit_thr_num = pool->live_thr_num - pool->min_thr_num;
            }
            int wait_exit_thr_num = pool->wait_exit_thr_num;    // pool->wait_exit_thr_num是可以在worker线程当中被修改的，所以这里取一个副本
            pthread_mutex_unlock(&pool->lock);
            for (int i = 0; i < wait_exit_thr_num; i++) {
                // 未工作的空闲线程 现在正在等待条件 pool->queue_not_empty条件变量出现，此时使用这个条件变量可以唤醒线程，
                // 处于worker中的工作线程要么在等待条件，要么在执行任务，这里唤醒之后，worker 线程会判断是否要销毁线程，如果需要销毁就会自己主动退出线程。
                pthread_cond_signal(&pool->queue_not_empty);
            }// 让工作的线程自杀
        }
#endif
        
    }
    pthread_exit(NULL);
}

void thread_exit(threadpool_t* pool){
    pthread_t tid = pthread_self();
    // 找到对应的线程 然后将对应的线程置为0
    for (int i = 0; i < pool->max_thr_num; ++i) {
        if(pool->threads[i] == tid){
            pool->threads[i] = 0;
            printf("thread_exit() called, %ld existing...\n", tid);
            break;
        }
    }
    pthread_exit(NULL);
    return ;
}