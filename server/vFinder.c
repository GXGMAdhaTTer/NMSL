#include "server.h"

/*虚拟文件系统*/
int vFinder(int netFd, MYSQL* conn, char* userName, threadPool_t* pThreadPool) {
    int ret = 0;

    //获取指令
    msgBox_t msgBox;
    bzero(&msgBox, sizeof(msgBox));
    char* cmd[ARGCMAX] = {0};
    recvn(netFd, &msgBox.length, sizeof(int));
    recvn(netFd, msgBox.buf, msgBox.length);

    char command[BUFFERSIZE] = {0};
    strcpy(command, msgBox.buf);

    //分析指令
    int cmdCount = myParse(command, cmd);
    puts(command);

    if (!strcmp(cmd[0], "exit")) {  // exit
        ARGS_CHECK(cmdCount, 1);
        //删除token表中数据
        int userId = getUserId(conn, userName);
        delFromUserToken(conn, userId);
        printf("用户%s已退出.\n", userName);
        addToLog(conn, userId, userName, "exit");
        close(netFd);
    } else if (!strcmp(cmd[0], "pwd")) {  // pwd
        ARGS_CHECK(cmdCount, 1);
        char pwd[BUFFERSIZE] = {0};
        int userId = getUserId(conn, userName);
        ret = myPwd(conn, userId, pwd);
        puts(pwd);
        //发送pwd
        bzero(&msgBox, sizeof(msgBox));
        strcpy(msgBox.buf, pwd);
        msgBox.length = strlen(pwd);
        sendn(netFd, &msgBox, sizeof(int) + msgBox.length);

        //发送pwd的f_id
        int pwdId = getPwdFId(conn, userId);
        sendn(netFd, &pwdId, sizeof(int));
        addToLog(conn, userId, userName, "pwd");
    } else if (!strcmp(cmd[0], "mkdir")) {
        ARGS_CHECK(cmdCount, 2);
        puts(command);
        char dirName[BUFFERSIZE] = {0};
        strcpy(dirName, cmd[1]);
        int userId = getUserId(conn, userName);
        int ret = myMkdir(conn, userId, dirName);
        // 发送ret, 成功发送0，失败发送-1
        sendn(netFd, &ret, sizeof(int));
        addToLog(conn, userId, userName, "mkdir");
    } else if (!strcmp(cmd[0], "gets")) {  // gets
        ARGS_CHECK(cmdCount, 2);
        puts(command);
        char fileName[BUFFERSIZE] = {0};
        strcpy(fileName, cmd[1]);

        //发送副端口号，之后等待连接
        int port = SUBPORT;
        sendn(netFd, &port, sizeof(int));
    } else if (!strcmp(cmd[0], "puts")) {  // puts
        ARGS_CHECK(cmdCount, 2);
        puts(command);
        char fileName[BUFFERSIZE] = {0};
        strcpy(fileName, cmd[1]);

        //发送副端口号，之后等待连接
        int port = SUBPORT;
        sendn(netFd, &port, sizeof(int));
    } else if (!strcmp(cmd[0], "ls")) {  // ls
        ARGS_CHECK(cmdCount, 1);
        char ls[BUFFERSIZE] = {0};

        int userId = getUserId(conn, userName);
        int myPwdId = getPwdFId(conn, userId);

        ret = myLs(conn, myPwdId, userId, ls);
        ERROR_CHECK(ret, -1, "myLs");

        bzero(&msgBox, sizeof(msgBox));
        strcpy(msgBox.buf, ls);
        msgBox.length = strlen(ls);
        sendn(netFd, &msgBox, sizeof(int) + msgBox.length);
        addToLog(conn, userId, userName, "ls");
    } else if (!strcmp(cmd[0], "cd")) {  // cd
        int ret = 0;
        ARGS_CHECK(cmdCount, 2);
        puts(command);
        char fileName[BUFFERSIZE] = {0};
        strcpy(fileName, cmd[1]);

        char pwd[BUFFERSIZE] = {0};
        myCd(conn, fileName, userName, pwd);

        //传送pwd和curF_id
        puts(pwd);
        bzero(&msgBox, sizeof(msgBox));
        strcpy(msgBox.buf, pwd);
        msgBox.length = strlen(pwd);
        sendn(netFd, &msgBox, sizeof(int) + msgBox.length);
        int userId = getUserId(conn, userName);
        int pwdId = getPwdFId(conn, userId);
        sendn(netFd, &pwdId, sizeof(int));
        addToLog(conn, userId, userName, "cd");
    } else if (!strcmp(cmd[0], "rm")) {  // rm
        ARGS_CHECK(cmdCount, 2);
        puts(command);

        char fileName[BUFFERSIZE] = {0};
        strcpy(fileName, cmd[1]);
        int userId = getUserId(conn, userName);
        int ret = myRm(conn, userId, fileName);
        // 发送ret, 成功发送0，失败发送-1
        sendn(netFd, &ret, sizeof(int));
        addToLog(conn, userId, userName, "rm");
    } else if (!strcmp(cmd[0], "rmdir")) {  // rmdir
        ARGS_CHECK(cmdCount, 2);
        puts(command);

        char fileName[BUFFERSIZE] = {0};
        strcpy(fileName, cmd[1]);
        int userId = getUserId(conn, userName);
        int ret = myRmdir(conn, userId, fileName);
        // 发送ret, 成功发送0，失败发送-1
        sendn(netFd, &ret, sizeof(int));
        addToLog(conn, userId, userName, "rmdir");
    } else {
        printf("传送命令错误!\n");
    }
    int userId = getUserId(conn, userName);
    expireUserToken(conn, userId);
}