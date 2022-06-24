#include "client.h"

/*初始化连接，成功返回0，失败返回-1*/
int connectInit(int* pSockFd, char* ip, int port) {
    *pSockFd = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_CHECK(*pSockFd, -1, "socket");

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    int ret = connect(*pSockFd, (struct sockaddr*)&addr, sizeof(addr));
    return ret;
}

/*开始界面，用户登录、注册和退出，返回用户登录状态*/
int userIntro(const int sockFd, char* userName, int userNameLen) {
    //先睡一会
    usleep(200);

    msgBox_t msgBox;
    char command[BUFFERSIZE] = {0};
    char passwd[BUFFERSIZE] = {0};
    bzero(userName, userNameLen);
    int userId = 0;
    //登录
    int isLogin = 0;
    int wrongTimes = 0;
    while (!isLogin) {
        int choice = 0;
        if (wrongTimes == 3) {
            printf("\033[0;30;41m 错误次数太多，请重新连接!\033[0m\n");
            choice = 3;
        } else {
            printf("请选择:\n");
            printf("1. 登录\n");
            printf("2. 注册\n");
            printf("3. 退出\n");
            scanf("%d", &choice);
        }

        switch (choice) {
            case 1:  //登录
                // 获取用户名和密码
                printf("用户名:");
                scanf("%s", userName);
                strcpy(passwd, getpass("密码:"));

                //发送命令 login userName
                bzero(&msgBox, sizeof(msgBox));
                sprintf(command, "login %s", userName);
                puts(command);
                msgBox.length = strlen(command);
                strcpy(msgBox.buf, command);
                sendn(sockFd, &msgBox, sizeof(int) + msgBox.length);

                int isUserExist = 0;
                //接收服务器查询结果，看是否有该用户
                recvn(sockFd, &isUserExist, sizeof(int));
                printf("%d\n", isUserExist);
                if (isUserExist == 0) {  //用户不存在
                    printf("用户不存在!\n");
                    continue;
                } else {  //用户存在
                    //接收盐值
                    char salt[BUFFERSIZE] = {0};
                    char cryptpasswd[BUFFERSIZE] = {0};
                    bzero(&msgBox, sizeof(msgBox));
                    recvn(sockFd, &msgBox.length, sizeof(int));
                    recvn(sockFd, msgBox.buf, msgBox.length);
                    strcpy(salt, msgBox.buf);

                    //发送cryptpasswd
                    bzero(&msgBox, sizeof(msgBox));
                    strcpy(cryptpasswd, crypt(passwd, salt));
                    msgBox.length = strlen(cryptpasswd);
                    strcpy(msgBox.buf, cryptpasswd);
                    sendn(sockFd, &msgBox, sizeof(int) + msgBox.length);

                    //接收密码是否正确
                    int isPasswdRight = 0;
                    recvn(sockFd, &isPasswdRight, sizeof(int));
                    if (isPasswdRight) {
                        printf("密码正确!\n");
                        isLogin = 1;
                        break;
                    } else {
                        printf("密码错误!\n");
                        ++wrongTimes;
                        continue;
                    }
                }
                break;
            case 2:  //注册
                // 获取用户名和密码
                printf("用户名:");
                scanf("%s", userName);
                printf("密码:");
                scanf("%s", passwd);

                //发送命令 signin userName
                bzero(&msgBox, sizeof(msgBox));
                sprintf(command, "signin %s", userName);
                msgBox.length = strlen(command);
                strcpy(msgBox.buf, command);
                sendn(sockFd, &msgBox, sizeof(int) + msgBox.length);

                //接收服务器查询结果，看是否有该用户
                recvn(sockFd, &isUserExist, sizeof(int));
                if (isUserExist == 1) {  //用户已经存在
                    printf("用户已存在!\n");
                    //接收盐值
                    char salt[BUFFERSIZE] = {0};
                    char cryptpasswd[BUFFERSIZE] = {0};
                    bzero(&msgBox, sizeof(msgBox));
                    recvn(sockFd, &msgBox.length, sizeof(int));
                    recvn(sockFd, msgBox.buf, msgBox.length);
                    strcpy(salt, msgBox.buf);

                    //发送cryptpasswd
                    bzero(&msgBox, sizeof(msgBox));
                    strcpy(cryptpasswd, crypt(passwd, salt));
                    msgBox.length = strlen(cryptpasswd);
                    strcpy(msgBox.buf, cryptpasswd);
                    sendn(sockFd, &msgBox, sizeof(int) + msgBox.length);

                    //接收密码是否正确
                    int isPasswdRight = 0;
                    recvn(sockFd, &isPasswdRight, sizeof(int));
                    if (isPasswdRight) {
                        printf("密码正确!\n");
                        isLogin = 1;
                        break;
                        ;
                    } else {
                        printf("密码错误!\n");
                        ++wrongTimes;
                        continue;
                    }
                } else {  //用户不存在，创建用户
                    //发送passwd
                    bzero(&msgBox, sizeof(msgBox));
                    msgBox.length = strlen(passwd);
                    strcpy(msgBox.buf, passwd);
                    sendn(sockFd, &msgBox, sizeof(int) + msgBox.length);
                    printf("用户创建成功!u_id = %d\n", userId);
                    isLogin = 1;
                    break;
                }
                break;
            case 3:  //退出
                bzero(&msgBox, sizeof(msgBox));
                strcpy(msgBox.buf, "quit");
                msgBox.length = strlen("quit");
                sendn(sockFd, &msgBox, sizeof(int) + msgBox.length);
                printf("再见!\n");
                exit(0);
                break;
            default:  //输入错误
                printf("输入错误.\n");
                break;
        }
    }
    if (isLogin) {
        printf("用户已登陆!\n");
    } else {
        printf("用户未登录。\n");
    }
    return isLogin;
}