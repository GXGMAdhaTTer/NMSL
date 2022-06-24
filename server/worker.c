#include "server.h"

void cleanFunc(void* arg) {
    threadPool_t* pThreadPool = (threadPool_t*)arg;
    pthread_mutex_unlock(&pThreadPool->taskQueue.mutex);
}

void* handEvent(void* arg) {
    threadPool_t* pThreadPool = (threadPool_t*)arg;
    int netFd;
    int taskType;
    char fileName[BUFFERSIZE] = {0};
    char userName[BUFFERSIZE] = {0};
    MYSQL* conn;

    while (1) {
        printf("I am free! tid = %lu\n", pthread_self());
        pthread_mutex_lock(&pThreadPool->taskQueue.mutex);
        pthread_cleanup_push(cleanFunc, (void*)pThreadPool);
        while (pThreadPool->taskQueue.size == 0 && pThreadPool->exitFlag == 0) {
            pthread_cond_wait(&pThreadPool->taskQueue.cond, &pThreadPool->taskQueue.mutex);
        }
        if (pThreadPool->exitFlag == 1) {
            printf("I am going to die! child thread.\n");
            pthread_exit(NULL);
        }
        netFd = pThreadPool->taskQueue.pFront->netFd;  //拿到队首的文件描述符
        conn = pThreadPool->taskQueue.pFront->conn;
        strcpy(userName, pThreadPool->taskQueue.pFront->userName);

        taskDequeue(&pThreadPool->taskQueue);
        pthread_cleanup_pop(1);

        printf("I am working! tid = %lu\n", pthread_self());

        //获取任务类型
        recvn(netFd, &taskType, sizeof(int));

        //获取当前文件路径
        int curF_id;
        recvn(netFd, &curF_id, sizeof(int));
        msgBox_t msgBox;

        if (taskType == GETS) {
            printf("GETS\n");
            // 获取文件名
            bzero(&msgBox, sizeof(msgBox));
            recvn(netFd, &msgBox.length, sizeof(int));
            recvn(netFd, msgBox.buf, msgBox.length);
            strcpy(fileName, msgBox.buf);
            puts(fileName);

            int userId = getUserId(conn, userName);

            int isExist = isFileExistInPwd(conn, fileName, userId, curF_id, COMMONFILE);
            sendn(netFd, &isExist, sizeof(int));

            if (isExist == 0) {
                //发送文件
                sendFile(netFd, fileName);
                printf("发送成功!\n");
            } else {
                printf("文件不存在!\n");
            }

            addToLog(conn, userId, userName, "exit");
        } else if (taskType == PUTS) {
            printf("PUTS\n");
            int userId = getUserId(conn, userName);

            //接收fileName和md5
            char fileName[BUFFERSIZE] = {0};
            char md5[BUFFERSIZE] = {0};
            bzero(&msgBox, sizeof(msgBox));
            recvn(netFd, &msgBox.length, sizeof(int));
            recvn(netFd, msgBox.buf, msgBox.length);
            strcpy(fileName, msgBox.buf);
            bzero(&msgBox, sizeof(msgBox));
            recvn(netFd, &msgBox.length, sizeof(int));
            recvn(netFd, msgBox.buf, msgBox.length);
            strcpy(md5, msgBox.buf);

            //发送是否存在
            int isExist = isFileExistInTable(conn, fileName, md5);
            sendn(netFd, &isExist, sizeof(int));

            if (isExist == 0) {  //如果不存在就接收文件
                recvFile(netFd, fileName);
            }
            printf("%s接收成功!\n", fileName);
            addFileToTable(conn, userId, fileName, md5, COMMONFILE, curF_id);
            addToLog(conn, userId, userName, "exit");
        }

        printf("done\n");
        close(netFd);
        printf("已断开netFd %d\n", netFd);
    }
}

/*生成子线程*/
int makeWorker(threadPool_t* pThreadPool) {
    for (int i = 0; i < pThreadPool->threadNum; i++) {
        pthread_create(&pThreadPool->tid[i], NULL, handEvent, (void*)pThreadPool);
    }
}