#include "server.h"

/*mysql初始化连接*/
MYSQL* mySqlInit() {
    MYSQL* conn = NULL;
    char* host = "localhost";
    char* user = "root";
    char* passwd = "gaoyaguo971118";
    char* db = "GXGNetDisk";

    //初始化mysql的连接句柄
    conn = mysql_init(NULL);

    //建立连接
    if (mysql_real_connect(conn, host, user, passwd, db, 0, NULL, 0) == NULL) {
        printf("error:%s\n", mysql_error(conn));
        return NULL;
    }
    mysql_query(conn, "set names 'utf8'");
    return conn;
}

/*检查是否是用户，如果是用户则传送salt和cryptpasswd,并返回u_id，失败返回-1，用户不存在返回0*/
int isUser(MYSQL* conn, const char* userName, char* salt, char* cryptpasswd) {
    int ret;
    char query[BUFFERSIZE] = {0};
    sprintf(query,
            "select u_id, u_salt, u_cryptpasswd from UserInfo where u_name = '%s';",
            userName);

    ret = mysql_query(conn, query);
    if (ret != 0) {
        printf("error query1: %s\n", mysql_error(conn));
        return -1;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result == NULL) {
        printf("error query2: %s\n", mysql_error(conn));
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(result);

    //用户不存在
    if (row == NULL) {
        return 0;
    }

    //用户存在
    int userId = atoi(row[0]);
    strcpy(salt, row[1]);
    strcpy(cryptpasswd, row[2]);
    mysql_free_result(result);

    return userId;  //返回u_id
}

/*根据userName获取userId*/
int getUserId(MYSQL* conn, const char* userName) {
    int ret;
    char query[BUFFERSIZE] = {0};
    sprintf(query, "select u_id from UserInfo where u_name = '%s';", userName);

    ret = mysql_query(conn, query);
    if (ret != 0) {
        printf("error query1: %s\n", mysql_error(conn));
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result == NULL) {
        printf("error query2: %s\n", mysql_error(conn));
        return 0;
    }

    MYSQL_ROW row = mysql_fetch_row(result);

    //用户不存在
    if (row == NULL) {
        return 0;
    }

    //用户存在
    int userId = atoi(row[0]);
    mysql_free_result(result);

    return userId;  //返回u_id
}

/*获取8位随机值*/
void getRandStr(char* mystr) {
    char str[SALTSIZE + 1] = {0};
    int i, flag;
    srand(time(NULL));
    for (i = 0; i < SALTSIZE; i++) {
        flag = rand() % 3;
        switch (flag) {
            case 0:
                str[i] = rand() % 26 + 'a';
                break;
            case 1:
                str[i] = rand() % 26 + 'A';
                break;
            case 2:
                str[i] = rand() % 10 + '0';
                break;
        }
    }
    strcpy(mystr, str);
}

/*添加用户，用于注册用户,成功返回0, 失败返回-1*/
int addUser(MYSQL* conn, const char* userName, const char* userPasswd) {
    char salt[20] = {0};
    char cryptpasswd[20] = {0};

    int ret;

    getRandStr(salt);
    strcpy(cryptpasswd, crypt(userPasswd, salt));

    char query[BUFFERSIZE] = {0};
    sprintf(query,
            "insert into UserInfo(u_name, u_salt, u_cryptpasswd, u_pwd) "
            "values('%s', '%s', '%s', '/%s/');",
            userName, salt, cryptpasswd, userName);

    ret = mysql_query(conn, query);
    if (ret != 0) {
        printf("error query1: %s\n", mysql_error(conn));
        return -1;
    }
    return 0;
}

/*向日志文件表中添加日志记录, 成功返回0, 失败返回-1*/
int addToLog(MYSQL* conn, const int user_id, const char* user_name, const char* op_name) {
    int ret;

    struct tm t;
    time_t now;
    time(&now);
    localtime_r(&now, &t);

    char query[BUFFERSIZE] = {0};
    sprintf(query,
            "insert into Log(u_id, u_name, op_name, op_time) "
            "values('%d', '%s', '%s', '%04d-%02d-%02d %02d:%02d:%02d');",
            user_id, user_name, op_name, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);

    ret = mysql_query(conn, query);
    if (ret != 0) {
        printf("error query1: %s\n", mysql_error(conn));
        return -1;
    }
    return 0;
}

/*创建用户根目录，成功返回0，失败返回-1*/
int addHomeDir(MYSQL* conn, char* userName) {
    int userId = getUserId(conn, userName);
    int ret;
    char query[BUFFERSIZE] = {0};
    sprintf(query,
            "insert into FileTable(parent_id, f_name, owner_id, f_md5, f_size, f_type) "
            "values(%d, '%s', %d, '%s', %d, %d);",
            0, userName, userId, "0", 0, DIRFILE);

    ret = mysql_query(conn, query);
    if (ret != 0) {
        printf("error query1: %s\n", mysql_error(conn));
        return -1;
    }
    return 0;
}

/*初始化用户当前路径，成功返回0，失败返回-1*/
int initUserPwd(MYSQL* conn, char* userName) {
    int userId = getUserId(conn, userName);
    int ret;
    char query[BUFFERSIZE] = {0};
    sprintf(query, "update UserInfo set u_pwd = '/%s/' where u_name = '%s';", userName, userName);

    ret = mysql_query(conn, query);
    if (ret != 0) {
        printf("error query1: %s\n", mysql_error(conn));
        return -1;
    }
    printf("nmb!\n");
    return 0;
}

/*更改用户当前路径，成功返回0，失败返回-1*/
int updateUserPwd(MYSQL* conn, const char* userName, char* curPwd) {
    int userId = getUserId(conn, userName);
    int ret;
    char query[BUFFERSIZE] = {0};
    sprintf(query, "update UserInfo set u_pwd = '%s' where u_name = '%s';", curPwd, userName);

    ret = mysql_query(conn, query);
    if (ret != 0) {
        printf("error query1: %s\n", mysql_error(conn));
        return -1;
    }
    return 0;
}

/*传入传出参数 mypwd 为当前路径，成功返回0，失败返回-1*/
int myPwd(MYSQL* conn, const int userId, char* mypwd) {
    int ret;
    char query[1024] = {0};
    sprintf(query, "select u_pwd from UserInfo where u_id = '%d';", userId);

    ret = mysql_query(conn, query);
    if (ret != 0) {
        printf("error query1: %s\n", mysql_error(conn));
        return -1;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result == NULL) {
        printf("error query2: %s\n", mysql_error(conn));
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(result);

    //用户不存在
    if (row == NULL) {
        return -1;
    }

    // 取出 u_pwd
    strcpy(mypwd, row[0]);

    mysql_free_result(result);

    return 0;
}

/*传入路径，传出的是文件夹的名字*/
int myLastWd(char* buf, char* lines[]) {
    char* word;
    if (buf[strlen(buf) - 1] == '\n') {
        buf[strlen(buf) - 1] = '\0';
    }

    char tmpbuf[1024] = {0};
    strcpy(tmpbuf, buf);

    word = strtok(tmpbuf, "/");
    lines[0] = word;
    int cnt = 1;
    while ((word = strtok(NULL, "/")) != NULL) {
        lines[cnt] = word;
        cnt++;
    }
    lines[cnt] = NULL;

    return cnt;
}

/*传入传出参数 Ls 为当前路径下文件夹与文件，成功返回0，失败返回-1*/
int myLs(MYSQL* conn, const int curF_id, const int userId, char* Ls) {
    int ret;
    char query[BUFFERSIZE] = {0};

    //查询文件夹名
    sprintf(query, "select f_name, f_type from FileTable where parent_id = %d and owner_id = %d;", curF_id, userId);

    ret = mysql_query(conn, query);

    if (ret != 0) {
        printf("error query1: %s\n", mysql_error(conn));
        return -1;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result == NULL) {
        printf("error query2: %s\n", mysql_error(conn));
        return -1;
    }

    int rows = mysql_num_rows(result);

    MYSQL_ROW row;

    while ((row = mysql_fetch_row(result)) != NULL) {
        if (atoi(row[1]) == DIRFILE) {
            char tempDir[BUFFERSIZE] = {0};
            sprintf(tempDir, "[%s]  ", row[0]);
            strcat(Ls, tempDir);
        } else {
            strcat(Ls, row[0]);  //先将文件夹名存入myLs
            strcat(Ls, "  ");
        }
    }
    printf("%s", Ls);
    mysql_free_result(result);
    return 0;
}

/*这个接口 用来 mkdir p_id，不允许一个用户新建已经使用过的文件夹名称*/
/*成功返回 f_id ( non-negative number )，失败返回 -1*/
int getPwdFId(MYSQL* conn, const int userId) {
    int ret;
    char pwd[1024] = {0};
    ret = myPwd(conn, userId, pwd);
    if (ret == -1) {
        printf("error: myPwd\n");
        return -1;
    }
    // printf("inner mypwd:  %s\n", mypwd);

    char* command[10] = {0};
    ret = myLastWd(pwd, command);
    if (ret == -1) {
        printf("error: myLastWd\n");
        return -1;
    }

    int i = 0;
    char fileName[BUFFERSIZE] = {0};
    while (command[i] != NULL) {
        bzero(fileName, 1024);
        strcpy(fileName, command[i++]);
        // puts(cmd[i++]);
    }
    // printf("inner filename:  %s\n", fileName);

    char query[1024] = {0};
    sprintf(query,
            "select f_id from FileTable "
            "where f_name = '%s' "
            "and owner_id = %d "
            "and f_type = %d;",
            fileName, userId, 0);

    ret = mysql_query(conn, query);
    if (ret != 0) {
        printf("error query1: %s\n", mysql_error(conn));
        return -1;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result == NULL) {
        printf("error query2: %s\n", mysql_error(conn));
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(result);

    //用户不存在
    if (row == NULL) {
        return -1;
    }

    int curFileId = atoi(row[0]);  // pwd f_id

    mysql_free_result(result);

    return curFileId;
}

/*用于获取文件id和md5，通过用户、文件名、文件类型、父id来获取文件id和md5*/
int getFileInfo(MYSQL* conn, const int userId, const char* fileName, const int fileType, const int parent_id, char* md5) {
    //执行查询
    int ret;
    char query[BUFFERSIZE] = {0};
    sprintf(query,
            "select f_id, f_md5 from FileTable "
            "where f_name = '%s' "
            "and parent_id = %d "
            "and owner_id = %d "
            "and f_type = %d;",
            fileName, parent_id, userId, fileType);
    ret = mysql_query(conn, query);

    if (ret != 0) {
        printf("error query1: %s\n", mysql_error(conn));
        return -1;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result == NULL) {
        printf("error query2: %s\n", mysql_error(conn));
        return -1;
    }

    int rowNums = mysql_num_rows(result);

    //如果文件不存在返回0
    if (rowNums == 0) {
        return 0;
    }

    //如果文件存在，返回f_id和md5
    MYSQL_ROW row = mysql_fetch_row(result);
    int f_id = atoi(row[0]);
    if (fileType == 1) { //如果是普通文件则返回md5，如果是文件夹则md5戊戌返回
        strcpy(md5, row[1]);
    }
    return f_id;
}

/*判断该用户该路径下有无此文件，有返回0，无返回-1*/
int isFileExistInPwd(MYSQL* conn, const char* fileName, int userId, int curF_id, int fileType) {
    int ret;
    char query[BUFFERSIZE] = {0};
    sprintf(query,
            "select * from FileTable "
            "where f_name = '%s' and owner_id = %d and parent_id = %d and f_type = %d;",
            fileName, userId, curF_id, fileType);
    ret = mysql_query(conn, query);

    if (ret != 0) {
        printf("error query1: %s\n", mysql_error(conn));
        return -1;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result == NULL) {
        printf("error query2: %s\n", mysql_error(conn));
        return -1;
    }

    int rowNums = mysql_num_rows(result);
    int colNums = mysql_num_fields(result);

    //如果文件不存在返回-1
    if (rowNums == 0) {
        return -1;
    } else {
        return 0;
    }
}

/*判断文件(只能用来判断f类型，不能用来判断文件夹)是否在文件表中，若存在则返回f_id，若不存在返回0，错误返回-1*/
int isFileExistInTable(MYSQL* conn, const char* fileName, char* md5) {
    int ret;
    char query[BUFFERSIZE] = {0};
    sprintf(query,
            "select f_id, owner_id from FileTable where f_name = '%s' and "
            "f_md5 = '%s';",
            fileName, md5);
    ret = mysql_query(conn, query);

    if (ret != 0) {
        printf("error query1: %s\n", mysql_error(conn));
        return -1;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result == NULL) {
        printf("error query2: %s\n", mysql_error(conn));
        return -1;
    }

    int rowNums = mysql_num_rows(result);
    int colNums = mysql_num_fields(result);

    //如果文件不存在返回0
    if (rowNums == 0) {
        return 0;
    }

    //如果文件存在，返回f_id
    MYSQL_ROW row = mysql_fetch_row(result);
    int f_id = atoi(row[0]);
    return f_id;
}

/*删除文件表中的文件，默认当前工作目录位parent_id所指的文件夹，即已知parent_id*/
/*删除成功返回0，失败返回-1*/
int DelFileFromTable(MYSQL* conn, const int userId, const char* fileName, const int fileType, const int parent_id) {
    int ret;
    char query[BUFFERSIZE] = {0};

    if (fileType == DIRFILE) {  //是文件夹，如果存在，查看文件夹中是否有文件，若有则防止删除
        //判断文件夹下是否有文件
        char md5[BUFFERSIZE] = {0};
        int f_id = getFileInfo(conn, userId, fileName, DIRFILE, parent_id, md5);
        printf("f_id = %d\n", f_id);
        sprintf(query, "select * from FileTable where parent_id = %d;", f_id);
        ret = mysql_query(conn, query);
        if (ret != 0) {
            printf("error query1: %s\n", mysql_error(conn));
            return -1;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (result == NULL) {
            printf("error query2: %s\n", mysql_error(conn));
            return -1;
        }

        int rowNums = mysql_num_rows(result);
        printf("%d\n", rowNums);

        //如果文件夹中存在文件，直接返回0
        if (rowNums > 1) {
            printf("文件夹中有文件存在，删除失败！\n");
            return -1;
        }

        //如果文件夹中不存在文件，删除文件夹
        sprintf(
            query,
            "delete from FileTable "
            "where parent_id = %d "
            "and f_name = '%s' "
            "and owner_id = %d "
            "and f_type = %d;",
            parent_id, fileName, userId, fileType);

        ret = mysql_query(conn, query);
        if (ret != 0) {
            printf("error query1: %s\n", mysql_error(conn));
            return -1;
        }
    } else if (fileType == COMMONFILE) {  //如果是普通文件,直接该文件记录
        sprintf(
            query,
            "delete from FileTable "
            "where parent_id = %d "
            "and f_name = '%s' "
            "and owner_id = %d "
            "and f_type = %d;",
            parent_id, fileName, userId, fileType);

        ret = mysql_query(conn, query);
        if (ret != 0) {
            printf("error query1: %s\n", mysql_error(conn));
            return -1;
        }
    }
    return 0;  //删除成功
}


/*向文件表中添加文件, 成功返回f_id, 失败返回0*/
int addFileToTable(MYSQL* conn, const int userId, const char* fileName, const char* md5, const int fileType, const int parent_id) {
    int ret;

    //判断是文件夹d或者是普通文件f
    long f_size;
    if (fileType == DIRFILE) {
        f_size = 0;
    } else if (fileType == COMMONFILE) {
        struct stat statBuf;
        int fd = open(fileName, O_RDONLY);
        ERROR_CHECK(fd, -1, "open");
        //实际上不会，因为获取md5也会打开文件
        if (fd == -1) {
            printf("文件打开错误。\n");
            return -1;
        }
        fstat(fd, &statBuf);
        f_size = statBuf.st_size;
    } else {
        printf("不支持本类型文件。\n");
        return -1;
    }

    char query[BUFFERSIZE] = {0};
    sprintf(query,
            "insert into FileTable(parent_id, f_name, owner_id, f_md5, f_size, f_type) "
            "values('%d', '%s', '%d', '%s', '%ld', '%d');",
            parent_id, fileName, userId, md5, f_size, fileType);

    ret = mysql_query(conn, query);
    if (ret != 0) {
        printf("error query1: %s\n", mysql_error(conn));
        return -1;
    }
    return 0;
}

/*成功返回 0，失败返回 -1*/
int myMkdir(MYSQL* conn, const int userId, char* dirName) {
    int fileId;
    fileId = getPwdFId(conn, userId);
    if (fileId == -1) {
        printf("error: getPwdFId \n");
        return -1;
    }
    // mkdir
    int parentId = fileId;
    char query[1024] = {0};
    sprintf(query,
            "insert into FileTable(parent_id, f_name, owner_id, f_md5, f_size, f_type) "
            "values(%d, '%s', %d, '%s', %d, %d );",
            parentId, dirName, userId, "0", 0, 0);

    int ret = mysql_query(conn, query);
    if (ret != 0) {
        printf("error query1: %s\n", mysql_error(conn));
        return -1;
    }

    return 0;
}



/*cd，成功返回0，失败返回-1*/
int myCd(MYSQL* conn, char* fileName, const char* userName, char* curPwd) {
    int userId = getUserId(conn, userName);
    int ret;
    ret = myPwd(conn, userId, curPwd);

    if (!strcmp(fileName, ".")) {  // cd .
        return 0;
    }

    //初始化路径栈
    char* routeStack[ROUTEDEPTH] = {0};               //路径栈
    int curDepth = myLastWd(curPwd, routeStack) - 1;  //目前用户深度

    if (!strcmp(fileName, "..")) {  // cd ..
        if (curDepth == 0) {        //已经到达根目录
            printf("已经是根目录。\n");
            ret = myPwd(conn, userId, curPwd);
        } else {  //返回上一层
            // routeStack出栈
            bzero(curPwd, BUFFERSIZE);
            routeStack[curDepth] = NULL;
            curDepth--;
            //组合路径栈形成当前路径
            for (int j = 0; j <= curDepth; j++) {
                strcat(curPwd, "/");
                strcat(curPwd, routeStack[j]);
            }
            strcat(curPwd, "/");
            updateUserPwd(conn, userName, curPwd);
        }
    } else if (!strcmp(fileName, "/")) {
        bzero(curPwd, BUFFERSIZE);
        sprintf(curPwd, "/%s/", userName);
        updateUserPwd(conn,userName, curPwd);
    } else {  // cd fileName
        ret = myPwd(conn, userId, curPwd);
        strcat(curPwd, fileName);
        strcat(curPwd, "/");
        puts(curPwd);
        curDepth++;
        updateUserPwd(conn, userName, curPwd);
    }
    return 0;
}

int myRm(MYSQL* conn, const int userId, char* fileName){
    int curF_id;
    curF_id = getPwdFId(conn, userId);
    if (curF_id == -1) {
        printf("error: getPwdFId \n");
        return -1;
    }
    int ret = DelFileFromTable(conn, userId, fileName, COMMONFILE, curF_id);
    return ret;
}

int myRmdir(MYSQL* conn, const int userId, char* dirName){
    int curF_id;
    curF_id = getPwdFId(conn, userId);
    if (curF_id == -1) {
        printf("error: getPwdFId \n");
        return -1;
    }
    int ret = DelFileFromTable(conn, userId, dirName, DIRFILE, curF_id);
    return ret;
}