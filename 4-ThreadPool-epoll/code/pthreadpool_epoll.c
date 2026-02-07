// #include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
// #include <string.h>
#include <unistd.h>
#include "threadpool.h"
// #include "wrap.h"

void taskFunc(void* arg)
{
    int num = *(int*)arg;
    printf("thread %ld is working, number = %d\n",
        pthread_self(), num);
    sleep(1);
	free(arg);
	arg = NULL;
}

int main(){

	// 在这里可以使用epoll模型，将对应客户端的工作函数写出来，
	// 然后每次做读写任务都可以在函数里做
	threadpool_t* pool = threadpool_create(10, 100, 100);
	int* num = NULL;
	for(int i = 0;i < 1000; ++i){
		num = (int*) malloc(sizeof(int));
		*num = i + 100;
		threadpool_add(pool, taskFunc, num);
	}
	sleep(30);
	threadpool_destroy(pool);
	return 0;
}