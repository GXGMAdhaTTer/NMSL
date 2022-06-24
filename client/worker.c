// worker.c

#include "client.h"

void cleanFunc(void* arg) {
    threadPool_t* pThreadPool = (threadPool_t*)arg;
    pthread_mutex_unlock(&pThreadPool->taskQueue.mutex);
}

void* handEvent(void* arg) {
    threadPool_t* pThreadPool = (threadPool_t*)arg;
    int port;
    char userName[BUFFERSIZE] = {0};
    char userToken[BUFFERSIZE] = {0};
    int taskType;
    char fileName[BUFFERSIZE] = {0};
    int curF_id;
    while (1) {
        // printf("I am free! tid = %lu\n", pthread_self());
        pthread_mutex_lock(&pThreadPool->taskQueue.mutex);
        pthread_cleanup_push(cleanFunc, (void*)pThreadPool);
        while (pThreadPool->taskQueue.size == 0 && pThreadPool->exitFlag == 0) {
            pthread_cond_wait(&pThreadPool->taskQueue.cond, &pThreadPool->taskQueue.mutex);
        }
        if (pThreadPool->exitFlag == 1) {  //线程退出
            // printf("I am going to die! child thread.\n");
            pthread_exit(NULL);
        }
        port = pThreadPool->taskQueue.pFront->port;                 //拿到队首的port
        strcpy(userName, pThreadPool->taskQueue.pFront->userName);  //拿到队首的userName
        printf("userName: %s\n", userName);
        strcpy(userToken, pThreadPool->taskQueue.pFront->userToken);  //拿到队首的userToken
        printf("userToken: %s\n", userToken);
        taskType = pThreadPool->taskQueue.pFront->taskType;         //拿到队首的任务类型
        strcpy(fileName, pThreadPool->taskQueue.pFront->fileName);  //拿到队首的传输文件名
        curF_id = pThreadPool->taskQueue.pFront->curF_id;
        taskDequeue(&pThreadPool->taskQueue);
        pthread_cleanup_pop(1);

        // printf("I am working! tid = %lu\n", pthread_self());

        //连接端口
        int netFd;
        connectInit(&netFd, IPADDRESS, port);
        printf("已连接netFd %d\n", netFd);

        sleep(1);

        //发送userName
        msgBox_t msgBox;
        bzero(&msgBox, sizeof(msgBox));
        strcpy(msgBox.buf, userName);
        msgBox.length = strlen(userName);
        sendn(netFd, &msgBox, sizeof(int) + msgBox.length);

        //发送userToken
        bzero(&msgBox, sizeof(msgBox));
        strcpy(msgBox.buf, userToken);
        msgBox.length = strlen(userToken);
        sendn(netFd, &msgBox, sizeof(int) + msgBox.length);

        usleep(300);
        int isLegal;
        recvn(netFd, &isLegal, sizeof(int));
        if (isLegal == -1) {
            printf("非法用户,请重新登录\n");
        } else {
            //判断任务类型，发送任务类型
            sendn(netFd, &taskType, sizeof(int));

            //发送当前文件路径
            sendn(netFd, &curF_id, sizeof(int));
            if (taskType == GETS) {
                //发送要接受的文件名
                bzero(&msgBox, sizeof(msgBox));
                strcpy(msgBox.buf, fileName);
                msgBox.length = strlen(fileName);
                sendn(netFd, &msgBox, sizeof(int) + msgBox.length);

                //接收是否存在该文件
                int isExist;
                recvn(netFd, &isExist, sizeof(int));

                if (isExist == 0) {
                    //接收文件
                    recvFile(netFd, fileName);
                    printf("%s 接收成功!\n", fileName);
                } else {
                    printf("文件不存在！\n");
                }

            } else if (taskType == PUTS) {
                int isExist = 0;
                //发送文件名
                bzero(&msgBox, sizeof(msgBox));
                strcpy(msgBox.buf, fileName);
                msgBox.length = strlen(fileName);
                sendn(netFd, &msgBox, sizeof(int) + msgBox.length);
                //发送md5
                char md5[BUFFERSIZE] = {0};
                Compute_file_md5(fileName, md5);
                bzero(&msgBox, sizeof(msgBox));
                strcpy(msgBox.buf, md5);
                msgBox.length = strlen(md5);
                sendn(netFd, &msgBox, sizeof(int) + msgBox.length);

                //接收是否存在
                recvn(netFd, &isExist, sizeof(int));
                if (isExist == 0) {  //若不存在就发送文件
                    sendFile(netFd, fileName);
                }

                printf("%s 发送成功!\n", fileName);
            }
        }

        close(netFd);
    }
}

int makeWorker(threadPool_t* pThreadPool) {
    for (int i = 0; i < pThreadPool->threadNum; i++) {
        pthread_create(&pThreadPool->tid[i], NULL, handEvent, (void*)pThreadPool);
    }
}