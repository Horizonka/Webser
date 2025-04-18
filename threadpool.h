#ifndef THREADPOOL_H
#define THREADPOOL_H

typedef struct threadpool threadpool_t;

// 创建线程池
threadpool_t* threadpool_create(int thread_num, int max_tasks);

// 往线程池添加任务
int threadpool_add(threadpool_t* pool, void (*function)(void*), void* arg);

// 销毁线程池
void threadpool_destroy(threadpool_t* pool);

#endif
