#include "client.h"

int main() {
    //连接服务端
    int sockFd;
    int ret = connectInit(&sockFd, IPADDRESS, MAINPORT);
    if(ret == -1){
        printf("连接失败!\n");
        exit(0);
    }
    printf("连接成功!\n");

    //初始化线程池
    int workerNum = THREADNUM;
    threadPool_t threadPool;
    threadPoolInit(&threadPool, workerNum);
    makeWorker(&threadPool);
    
    // msgBox_t msgBox;
    char* cmd[ARGCMAX] = {0};
    char userName[BUFFERSIZE] = {0};
    int isLogin = userIntro(sockFd, userName, BUFFERSIZE);
    int isIn = vShell(sockFd, userName, &threadPool);

    close(sockFd);
}
