#include "server.h"
/*添加监听*/
int epollAdd(int fd, int epfd){
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = fd;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
    ERROR_CHECK(ret, -1, "epoll_ctl");
    return 0;
}

/*取消监听*/
int epollDel(int fd, int epfd){
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = fd;
    int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &event);
    ERROR_CHECK(ret, -1, "epoll_ctl");
    return 0;
}

/*任务队列入队*/
int taskEnqueue(taskQueue_t* pTaskQueue, int netFd, MYSQL* conn, char* userName) {
    task_t* pTask = (task_t*)calloc(1, sizeof(task_t));

    pTask->netFd = netFd;
    pTask->conn = conn;
    strcpy(pTask->userName, userName);

    if (pTaskQueue->size == 0) {
        pTaskQueue->pFront = pTask;
        pTaskQueue->pRear = pTask;
    }
    else {
        pTaskQueue->pRear->pNext = pTask;
        pTaskQueue->pRear = pTask;
    }

    pTaskQueue->size++;
    return 0;
}

/*任务队列出队*/
int taskDequeue(taskQueue_t* pTaskQueue){
    task_t* pCur = pTaskQueue->pFront;
    pTaskQueue->pFront = pCur->pNext;
    free(pCur);
    pTaskQueue->size--;
    return 0;
}

/*线程池初始化*/
int threadPoolInit(threadPool_t* pThreadPool, int workerNum){
    pThreadPool->threadNum = workerNum;
    pThreadPool->tid=(pthread_t*)calloc(workerNum, sizeof(pthread_t));
    pThreadPool->taskQueue.pFront = NULL;
    pThreadPool->taskQueue.pRear = NULL;
    pThreadPool->taskQueue.size = 0;
    pthread_mutex_init(&pThreadPool->taskQueue.mutex, NULL);
    pthread_cond_init(&pThreadPool->taskQueue.cond, NULL);
    pThreadPool->exitFlag = 0;
}
