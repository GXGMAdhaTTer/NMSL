#include "server.h"

/*用户登录注册退出引导函数，返回是否登录，userName传入传出参数获取登录用户名*/
int clientIntro(int netFd, MYSQL* conn, char* userName) {
    int isLogin = 0;

    msgBox_t msgBox;
    char* cmd[ARGCMAX] = {0};

    while (!isLogin) {
        //先接收账户信息
        bzero(&msgBox, sizeof(msgBox));
        recvn(netFd, &msgBox.length, sizeof(int));
        recvn(netFd, msgBox.buf, msgBox.length);

        bzero(userName, BUFFERSIZE);
        char command[BUFFERSIZE] = {0};
        strcpy(command, msgBox.buf);
        puts(command);

        //分析是login signin quit
        int cmdCount = myParse(command, cmd);

        if (!strcmp(cmd[0], "quit")) {  //收到quit退出连接
            printf("客户端放弃连接\n");
            close(netFd);
            break;
        } else if (!strcmp(cmd[0], "login")) {  //收到login userName
            ARGS_CHECK(cmdCount, 2);
            strcpy(userName, cmd[1]);
            puts(userName);

            //查询用户是否存在
            int isUserExist = 0;
            char salt[BUFFERSIZE] = {0};
            char cryptpasswd[BUFFERSIZE] = {0};
            if (isUser(conn, userName, salt, cryptpasswd)) {  //用户存在，发送isUserExist
                isUserExist = 1;
                int isPasswdRight = 0;
                sendn(netFd, &isUserExist, sizeof(int));

                //发送盐值
                bzero(&msgBox, sizeof(msgBox));
                strcpy(msgBox.buf, salt);
                msgBox.length = strlen(salt);
                sendn(netFd, &msgBox, sizeof(int) + msgBox.length);

                //接收cryptpasswd
                bzero(&msgBox, sizeof(msgBox));
                recvn(netFd, &msgBox.length, sizeof(int));
                recvn(netFd, msgBox.buf, msgBox.length);
                if (strcmp(cryptpasswd, msgBox.buf)) {
                    //传送密码错误
                    sendn(netFd, &isPasswdRight, sizeof(int));
                    continue;
                } else {
                    //传送密码正确
                    isPasswdRight = 1;
                    sendn(netFd, &isPasswdRight, sizeof(int));
                    isLogin = 1;
                    break;
                }
            } else {  //用户不存在
                sendn(netFd, &isUserExist, sizeof(int));
                printf("用户不存在!\n");
                continue;
            }
        } else if (!strcmp(cmd[0], "signin")) {  //收到signin userName
            ARGS_CHECK(cmdCount, 2);
            strcpy(userName, cmd[1]);
            puts(userName);

            //查询用户是否存在
            puts(cmd[0]);
            puts(cmd[1]);
            int isUserExist = 0;
            char salt[BUFFERSIZE] = {0};
            char cryptpasswd[BUFFERSIZE] = {0};
            if (isUser(conn, userName, salt, cryptpasswd)) {  //用户存在，发送isUserExist
                isUserExist = 1;
                int isPasswdRight = 0;
                sendn(netFd, &isUserExist, sizeof(int));
                printf("用户已存在\n");
                //发送盐值
                bzero(&msgBox, sizeof(msgBox));
                strcpy(msgBox.buf, salt);
                msgBox.length = strlen(salt);
                sendn(netFd, &msgBox, sizeof(int) + msgBox.length);

                //接收cryptpasswd
                bzero(&msgBox, sizeof(msgBox));
                recvn(netFd, &msgBox.length, sizeof(int));
                recvn(netFd, msgBox.buf, msgBox.length);
                if (strcmp(cryptpasswd, msgBox.buf)) {
                    //传送密码错误
                    sendn(netFd, &isPasswdRight, sizeof(int));
                    continue;
                } else {
                    //传送密码正确
                    isPasswdRight = 1;
                    sendn(netFd, &isPasswdRight, sizeof(int));
                    isLogin = 1;
                    break;
                }
            } else {  //用户不存在，可以创建，接收用户密码
                sendn(netFd, &isUserExist, sizeof(int));
                char passwd[BUFFERSIZE] = {0};
                bzero(&msgBox, sizeof(msgBox));
                recvn(netFd, &msgBox.length, sizeof(int));
                recvn(netFd, msgBox.buf, msgBox.length);
                strcpy(passwd, msgBox.buf);

                puts(userName);
                //添加到数据库中
                int ret = addUser(conn, userName, passwd);
                if (ret == -1) {
                    printf("创建错误。\n");
                    continue;
                }

                //添加用户根目录
                ret = addHomeDir(conn, userName);
                isLogin = 1;
                break;
            }
        }
    }
    return isLogin;
}

/*初始化用户数组，成功返回0*/
int initClients(clientBox_t* clients, int* clientNum) {
    bzero(clients, CLIENTMAX * sizeof(clientBox_t));
    for (int i = 0; i < CLIENTMAX; i++) {
        clients[i].netFd = -1;
        clients[i].slotIndex = -1;
        // userId和userName已初始化为0
    }
    *clientNum = 0;
    return 0;
}

/*添加用户到用户数组，成功返回0，失败返回-1*/
int addToClients(clientBox_t* clients, int netFd, char* userName, int userId, int timeIndex, slotList_t** slotMap, int epfd, int* pClientNum) {
    int i;
    for (i = 0; i < CLIENTMAX; i++) {
        if (clients[i].netFd == -1) {
            break;
        }
    }
    if (i == CLIENTMAX) {
        printf("用户已满!\n");
        return -1;
    }
    clients[i].netFd = netFd;
    strcpy(clients[i].userName, userName);
    clients[i].userId = userId;
    clients[i].slotIndex = (TIMEOUT + timeIndex - 1) % TIMEOUT;  //给slotIndex赋初值
    attachToSlot(userId, netFd, clients[i].slotIndex, slotMap);  //加到slotMap中对应index位置
    epollAdd(clients[i].netFd, epfd);
    ++(*pClientNum);
    return 0;
}

/*将用户从用户数组删除，成功返回0，失败返回-1*/
int delFromClients(clientBox_t* clients, int userId) {
    int i;
    for (i = 0; i < CLIENTMAX; i++) {
        if (clients[i].userId == userId) {
            break;
        }
    }
    if (i == CLIENTMAX) {
        printf("不存在该用户!\n");
        return -1;
    }
    close(clients[i].netFd);
    bzero(&clients[i], sizeof(clientBox_t));
    clients[i].netFd = -1;
    clients[i].slotIndex = -1;
}
