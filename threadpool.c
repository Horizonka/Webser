#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "threadpool.h"

typedef struct task {
    void (*function)(void*);
    void* arg;
    struct task* next;
} task_t;

struct threadpool {
    pthread_mutex_t lock;// 互斥锁，用于保护共享资源。
    pthread_cond_t notify;//条件变量，用于线程间的同步。
    pthread_t* threads;//存储线程的数组。
    task_t* task_head;
    task_t* task_tail;// 任务队列的头尾指针。
    int thread_num;// 线程池中线程的数量。
    int task_count;// 当前任务队列中的任务数量。
    int max_tasks;//任务队列的最大容量。
    int shutdown;//标志线程池是否正在关闭。
};

void* thread_worker(void* arg) {
    threadpool_t* pool = (threadpool_t*)arg;
    while (1) {
        pthread_mutex_lock(&pool->lock);
        while (pool->task_count == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->notify, &pool->lock);
        }

        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->lock);
            pthread_exit(NULL);
        }

        task_t* task = pool->task_head;//从任务队列头部取出一个任务
        if (task) {
            pool->task_head = task->next;//更新任务队列的头尾指针，并减少任务计数
            if (pool->task_head == NULL)
                pool->task_tail = NULL;
            pool->task_count--;
        }
        pthread_mutex_unlock(&pool->lock);

        if (task) {//如果任务不为空，则调用任务的函数指针执行任务。
            task->function(task->arg);
            //free(task->arg); //
            free(task);
        }
    }
}

threadpool_t* threadpool_create(int thread_num, int max_tasks) {
    threadpool_t* pool = (threadpool_t*)malloc(sizeof(threadpool_t));
    if (!pool) return NULL;

    pool->thread_num = thread_num;
    pool->max_tasks = max_tasks;
    pool->task_count = 0;
    pool->task_head = pool->task_tail = NULL;
    pool->shutdown = 0;

    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->notify, NULL);

    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * thread_num);
    for (int i = 0; i < thread_num; ++i) {
        pthread_create(&pool->threads[i], NULL, thread_worker, (void*)pool);
    }
    return pool;
}

int threadpool_add(threadpool_t* pool, void (*function)(void*), void* arg) {
    task_t* task = (task_t*)malloc(sizeof(task_t));
    if (!task) return -1;
    task->function = function;
    task->arg = arg;
    task->next = NULL;

    pthread_mutex_lock(&pool->lock);

    if (pool->task_count >= pool->max_tasks) {
        pthread_mutex_unlock(&pool->lock);
        free(task);
        //pthread_exit(NULL);
        return -1;
    }

    if (pool->task_tail) {
        pool->task_tail->next = task;
        pool->task_tail = task;
    } else {
        pool->task_head = pool->task_tail = task;
    }
    pool->task_count++;

    pthread_cond_signal(&pool->notify);//调用 pthread_cond_signal 唤醒一个等待的线程。
    pthread_mutex_unlock(&pool->lock);
    return 0;
}

void threadpool_destroy(threadpool_t* pool) {
    if (!pool) return;

    pthread_mutex_lock(&pool->lock);
    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->notify);
    pthread_mutex_unlock(&pool->lock);

    for (int i = 0; i < pool->thread_num; ++i) {
        pthread_join(pool->threads[i], NULL);
    }

    while (pool->task_head) {
        task_t* tmp = pool->task_head;
        pool->task_head = tmp->next;
        free(tmp);
    }

    free(pool->threads);
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->notify);
    free(pool);
}
