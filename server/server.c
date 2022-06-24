#include "server.h"
#include <gxgfunc.h>

int exitPipe[2];

void sigFunc(int signum) {
    printf("signum = %d\n", signum);
    write(exitPipe[1], "1", 1);
    printf("Parent process is going to die!\n");
}

int main() {
    //退出管道
    pipe(exitPipe);

    if (fork() != 0) {
        //主进程
        close(exitPipe[0]);
        signal(SIGUSR1, sigFunc);
        wait(NULL);
        exit(0);
    }

    //子进程
    close(exitPipe[1]);

    //初始化主端口
    int mainSockFd;
    tcpInit(&mainSockFd, IPADDRESS, MAINPORT);

    //初始化副端口
    int subSockFd;
    tcpInit(&subSockFd, IPADDRESS, SUBPORT);

    //初始化mySQL连接
    MYSQL* conn = mySqlInit();
    ERROR_CHECK(conn, NULL, "mySqlInit");

    int workerNum = THREADNUM;

    //初始化线程池
    threadPool_t threadPool;
    threadPoolInit(&threadPool, workerNum);
    makeWorker(&threadPool);

    char* cmd[ARGCMAX] = {0};

    //监听mainSockFd,subSockFd和exitPipe
    int epfd = epoll_create(1);
    epollAdd(mainSockFd, epfd);
    epollAdd(subSockFd, epfd);
    epollAdd(exitPipe[0], epfd);

    /*客户端数组，存放客户端的netFd*/
    clientBox_t clients[CLIENTMAX];
    int clientNum;
    initClients(clients, &clientNum);

    /*初始化slotMap*/
    int timeIndex = 0;
    slotList_t* slotMap[TIMEOUT] = {0};
    initSlotMap(slotMap, TIMEOUT);

    struct epoll_event readyArr[EPOLLNUM];
    while (1) {
        int readyNum = epoll_wait(epfd, readyArr, EPOLLNUM, 1000);
        // printf("epoll_wait returns.\n");
        if (readyNum == 0 && clientNum > 0) {
            clearSlot(timeIndex, slotMap, clients, &clientNum);
            timeIndex = (timeIndex + 1) % TIMEOUT;
        }
        for (int i = 0; i < readyNum; i++) {
            printf("readyNum.\n");
            if (readyArr[i].data.fd == mainSockFd) {
                //接收连接
                int netFd = accept(mainSockFd, NULL, NULL);
                char userName[BUFFERSIZE] = {0};
                //引入客户端登录
                int isLogin = clientIntro(netFd, conn, userName);
                printf("isLogin = %d\n", isLogin);
                if (isLogin == 0) {  //未登录，关闭netFd
                    close(netFd);
                    printf("用户退出！\n");
                } else {  //已登陆
                    //生成该用户token并写入表中
                    char userToken[BUFFERSIZE] = {0};
                    encodeToken(userName, userToken);
                    int userId = getUserId(conn, userName);
                    InserIntoUserToken(conn, userId, userToken);
                    // 发送userToken
                    msgBox_t msgBox;
                    bzero(&msgBox, sizeof(msgBox));
                    strcpy(msgBox.buf, userToken);
                    msgBox.length = strlen(userToken);
                    sendn(netFd, &msgBox, sizeof(int) + msgBox.length);

                    //把netFd加入到客户端数组并监听
                    addToClients(clients, netFd, userName, userId, (TIMEOUT + timeIndex - 1) % TIMEOUT, slotMap, epfd, &clientNum);

                    //初始化用户当前路径
                    int ret = initUserPwd(conn, userName);
                    if (ret == -1) {
                        printf("初始化路径失败！\n");
                    }
                    printf("连接成功!\n");
                    addToLog(conn, userId, userName, "Login");
                }
            } else if (readyArr[i].data.fd == subSockFd) {  //副端口收到连接请求
                printf("副端口有连接！\n");
                //接收连接
                int netFd = accept(subSockFd, NULL, NULL);
                printf("已接收连接\n");

                //接收userName
                char userName[BUFFERSIZE] = {0};
                msgBox_t msgBox;
                bzero(&msgBox, sizeof(msgBox));
                recvn(netFd, &msgBox.length, sizeof(int));
                recvn(netFd, msgBox.buf, msgBox.length);
                strcpy(userName, msgBox.buf);
                // printf("userName: %s\n", userName);

                //接收userToken
                char userToken[BUFFERSIZE] = {0};
                bzero(&msgBox, sizeof(msgBox));
                recvn(netFd, &msgBox.length, sizeof(int));
                recvn(netFd, msgBox.buf, msgBox.length);
                strcpy(userToken, msgBox.buf);
                // printf("userToken: %s\n", userToken);

                //查看userName和Token是否匹配,发送isLegal
                int isLegal = decodeToken(userName, userToken);
                sendn(netFd, &isLegal, sizeof(int));
                if (isLegal == FAILED) {
                    printf("illegalUser!\n");
                    close(netFd);
                } else {
                    pthread_mutex_lock(&threadPool.taskQueue.mutex);
                    taskEnqueue(&threadPool.taskQueue, netFd, conn, userName);
                    printf("New task!\n");
                    pthread_cond_signal(&threadPool.taskQueue.cond);
                    pthread_mutex_unlock(&threadPool.taskQueue.mutex);
                }
            } else if (readyArr[i].data.fd == exitPipe[0]) {  //退出管道
                printf("child process, threadPool is going to die.\n");
                threadPool.exitFlag = 1;
                pthread_cond_broadcast(&threadPool.taskQueue.cond);
                for (int j = 0; j < workerNum; j++) {
                    pthread_join(threadPool.tid[j], NULL);
                }
                pthread_exit(NULL);
            } else {  //监听客户端netFd
                for (int j = 0; j < clientNum; j++) {
                    if (readyArr[i].data.fd == clients[j].netFd) {
                        vFinder(clients[j].netFd, conn, clients[j].userName, &threadPool);
                        //从原来的slot移除
                        removeFromSlot(clients[j].userId, clients[j].netFd, clients[j].slotIndex, slotMap);
                        //更新客户的slotIndex
                        clients[j].slotIndex = (TIMEOUT + timeIndex - 1) % TIMEOUT;
                        //挂到新的slot
                        attachToSlot(clients[j].userId, clients[j].netFd, clients[j].slotIndex, slotMap);
                    }
                }
            }
        }
    }
    //关闭mysql连接
    mysql_close(conn);
    return 0;
}