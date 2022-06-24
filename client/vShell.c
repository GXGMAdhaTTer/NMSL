#include "client.h"

/*虚拟终端*/
int vShell(int sockFd, char* userName, threadPool_t* pThreadPool) {
    msgBox_t msgBox;
    char userToken[BUFFERSIZE] = {0};
    char command[BUFFERSIZE] = {0};
    char passwd[BUFFERSIZE] = {0};
    char pwd[BUFFERSIZE] = {0};
    int curF_id = 0;

    sprintf(pwd, "/%s/", userName);  //初始化当前路径

    //获取userToken
    bzero(&msgBox, sizeof(msgBox));
    recvn(sockFd, &msgBox.length, sizeof(int));
    recvn(sockFd, msgBox.buf, msgBox.length);
    strcpy(userToken, msgBox.buf);

    //获取当前pwd和文件夹id
    bzero(&msgBox, sizeof(msgBox));
    strcpy(msgBox.buf, "pwd");
    msgBox.length = strlen("pwd");
    sendn(sockFd, &msgBox, sizeof(int) + msgBox.length);
    bzero(&msgBox, sizeof(msgBox));
    recvn(sockFd, &msgBox.length, sizeof(int));
    recvn(sockFd, msgBox.buf, msgBox.length);
    bzero(pwd, BUFFERSIZE);
    strcpy(pwd, msgBox.buf);
    recvn(sockFd, &curF_id, sizeof(int));

    int isRoyal = 0;  // 是否是会员
    while (1) {
        //通过用户名假装自己是
        printf("\033[1;35m%s@NetDisk:%s# \033[m", userName, pwd);
        fflush(stdout);

        //获取命令行
        char command[BUFFERSIZE] = {0};
        int ret = read(STDIN_FILENO, command, BUFFERSIZE);
        if (command[0] == '\n') {
            continue;
        }

        //分析指令
        char* cmd[ARGCMAX] = {0};
        int cmdCount = myParse(command, cmd);

        if (!strcmp(cmd[0], "vip")) {
            if (cmdCount != 2) {
                printf("参数错误!\n");
                continue;
            } else if (atoi(cmd[1]) == 1) {
                sleepTime = 2000;
            } else if (atoi(cmd[1]) == 2) {
                sleepTime = 500;
            } else if (atoi(cmd[1]) == 3) {
                sleepTime = 0;
            }
            isRoyal = 1;

        } else if (!strcmp(cmd[0], "exit")) {  //退出命令
            if (cmdCount != 1) {
                printf("参数错误!\n");
                continue;
            }
            bzero(msgBox.buf, BUFFERSIZE);
            strcpy(msgBox.buf, "exit");
            msgBox.length = strlen("exit");
            sendn(sockFd, &msgBox, sizeof(int) + msgBox.length);
            printf("再见！\n");
            break;
        } else if (!strcmp(cmd[0], "pwd")) {  // pwd
            if (cmdCount != 1) {
                printf("参数错误!\n");
                continue;
            }
            //发送pwd命令
            bzero(&msgBox, sizeof(msgBox));
            strcpy(msgBox.buf, "pwd");
            msgBox.length = strlen("pwd");
            sendn(sockFd, &msgBox, sizeof(int) + msgBox.length);

            //接收pwd
            bzero(&msgBox, sizeof(msgBox));
            recvn(sockFd, &msgBox.length, sizeof(int));
            recvn(sockFd, msgBox.buf, msgBox.length);
            bzero(pwd, BUFFERSIZE);
            strcpy(pwd, msgBox.buf);
            puts(pwd);

            //接收curF_id
            recvn(sockFd, &curF_id, sizeof(int));
        } else if (!strcmp(cmd[0], "mkdir")) {
            if (cmdCount != 2) {
                printf("参数错误!\n");
                continue;
            }
            char fileName[BUFFERSIZE] = {0};
            strcpy(fileName, cmd[1]);

            //发送mkdir fileName 命令
            bzero(msgBox.buf, BUFFERSIZE);
            strcpy(msgBox.buf, command);
            msgBox.length = strlen(command);
            sendn(sockFd, &msgBox, sizeof(int) + msgBox.length);

            //接收是否成功
            int isSuccess;
            recvn(sockFd, &isSuccess, sizeof(int));
            if (!isSuccess) {
                printf("创建成功！\n");
            } else {
                printf("创建失败！\n");
            }
        } else if (!strcmp(cmd[0], "gets")) {  // gets
            if (cmdCount != 2) {
                printf("参数错误!\n");
                continue;
            }
            char fileName[BUFFERSIZE] = {0};
            strcpy(fileName, cmd[1]);

            //发送gets fileName 命令
            bzero(msgBox.buf, BUFFERSIZE);
            strcpy(msgBox.buf, command);
            msgBox.length = strlen(command);
            sendn(sockFd, &msgBox, sizeof(int) + msgBox.length);

            //接收端口号
            int port;
            recvn(sockFd, &port, sizeof(int));
            printf("端口号 = %d\n", port);

            //添加任务队列，任务中要包括任务类型GETS，文件名fileName，连接的端口号port
            pthread_mutex_lock(&pThreadPool->taskQueue.mutex);
            taskEnqueue(&pThreadPool->taskQueue, port, userName, userToken, GETS, fileName, curF_id);
            printf("New task!\n");
            pthread_cond_signal(&pThreadPool->taskQueue.cond);
            pthread_mutex_unlock(&pThreadPool->taskQueue.mutex);
        } else if (!strcmp(cmd[0], "puts")) {  // puts
            if (cmdCount != 2) {
                printf("参数错误!\n");
                continue;
            }
            char fileName[BUFFERSIZE] = {0};
            strcpy(fileName, cmd[1]);

            //发送puts fileName 命令
            bzero(msgBox.buf, BUFFERSIZE);
            strcpy(msgBox.buf, command);
            msgBox.length = strlen(command);
            sendn(sockFd, &msgBox, sizeof(int) + msgBox.length);

            //接收端口号
            int port;
            recvn(sockFd, &port, sizeof(int));
            printf("端口号 = %d\n", port);

            //添加任务队列，任务中要包括userName,任务类型GETS，文件名fileName，连接的端口号port
            pthread_mutex_lock(&pThreadPool->taskQueue.mutex);
            taskEnqueue(&pThreadPool->taskQueue, port, userName, userToken, PUTS, fileName, curF_id);
            printf("New task!\n");
            pthread_cond_signal(&pThreadPool->taskQueue.cond);
            pthread_mutex_unlock(&pThreadPool->taskQueue.mutex);
        } else if (!strcmp(cmd[0], "ls")) {  // ls
            if (cmdCount != 1) {
                printf("参数错误!\n");
                continue;
            }
            //发送ls命令
            bzero(&msgBox, sizeof(msgBox));
            strcpy(msgBox.buf, "ls");
            msgBox.length = strlen("ls");
            sendn(sockFd, &msgBox, sizeof(int) + msgBox.length);

            char ls[BUFFERSIZE] = {0};

            recvn(sockFd, &msgBox.length, sizeof(int));
            recvn(sockFd, msgBox.buf, msgBox.length);
            strcpy(ls, msgBox.buf);

            puts(ls);                        //文件夹名在|前，文件名在|后
        } else if (!strcmp(cmd[0], "cd")) {  // cd
            if (cmdCount != 2) {
                printf("参数错误!\n");
                continue;
            }

            //发送cd fileName 命令
            bzero(msgBox.buf, BUFFERSIZE);
            strcpy(msgBox.buf, command);
            msgBox.length = strlen(command);
            sendn(sockFd, &msgBox, sizeof(int) + msgBox.length);

            bzero(pwd, BUFFERSIZE);
            recvn(sockFd, &msgBox.length, sizeof(int));
            recvn(sockFd, msgBox.buf, msgBox.length);
            strcpy(pwd, msgBox.buf);
            recvn(sockFd, &curF_id, sizeof(int));
        } else if (!strcmp(cmd[0], "rm")) {  // rm
            if (cmdCount != 2) {
                printf("参数错误!\n");
                continue;
            }
            //发送rm fileName 命令
            bzero(msgBox.buf, BUFFERSIZE);
            strcpy(msgBox.buf, command);
            msgBox.length = strlen(command);
            sendn(sockFd, &msgBox, sizeof(int) + msgBox.length);

            //接收是否成功
            int isSuccess;
            recvn(sockFd, &isSuccess, sizeof(int));
            if (!isSuccess) {
                printf("删除成功！\n");
            } else {
                printf("删除失败！\n");
            }
        } else if (!strcmp(cmd[0], "rmdir")) {  // rmdir
            if (cmdCount != 2) {
                printf("参数错误!\n");
                continue;
            }
            //发送rm fileName 命令
            bzero(msgBox.buf, BUFFERSIZE);
            strcpy(msgBox.buf, command);
            msgBox.length = strlen(command);
            sendn(sockFd, &msgBox, sizeof(int) + msgBox.length);

            //接收是否成功
            int isSuccess;
            recvn(sockFd, &isSuccess, sizeof(int));
            if (!isSuccess) {
                printf("删除成功！\n");
            } else {
                printf("删除失败！\n");
            }
        } else {
            printf("输入错误!\n");
        }

        if (isRoyal == 0) {
            printf("\033[5;37;42m 温馨提示：超级会员专享极速下载特权!!! %s 先生，即刻升级会员等级！\033[0m\n", userName);
        }
    }
}
