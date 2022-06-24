#include "myShtCmd.h"
/*返回指令个数，传入传出参数是cmd指针数组*/
int myLastWd(char* buf, char* lines[]) {
    char* word;
    if (buf[strlen(buf) - 1] == '\n') {
        buf[strlen(buf) - 1] = '\0';
    }

    char tmpbuf[BUFFERSIZE] = {0};
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

/*这个接口不知道有什么用处，不允许一个用户新建已经使用过的文件夹名称*/
/*成功返回 parent_id ( non-negative number )，失败返回 -1*/
int getParentId(MYSQL* conn, const int userId) {
    int ret;
    char* fileName;
    ret = getPwdLast(conn, userId, fileName);
    if (ret == -1) {
        printf("error: getPwdLast \n");
        return -1;
    }

    char query[BUFFERSIZE] = {0};
    sprintf(query,
            "select parent_id from FileTable "
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
    int curParentId = atoi(row[0]);  // pwd p_id

    mysql_free_result(result);

    return curParentId;
}

/*这个接口 用来 mkdir p_id，不允许一个用户新建已经使用过的文件夹名称*/
/*成功返回 f_id ( non-negative number )，失败返回 -1*/
/*返回pwd的f_id*/
int getPwdId(MYSQL* conn, const int userId) {
    int ret;
    char* fileName;
    ret = getPwdLast(conn, userId, fileName);
    printf("nmb.\n");

    if (ret != 0) {
        printf("error: getPwdLast \n");
        return -1;
    }

    char query[BUFFERSIZE] = {0};
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

/*传入传出参数 fileName 为 pwd最后一个文件夹，成功返回0，失败返回-1*/
int getPwdLast(MYSQL* conn, const int userId, char* fileName) {
    int ret;
    char query[BUFFERSIZE] = {0};
    sprintf(query,
            "select u_pwd from UserInfo where u_id = '%d';", userId);

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
    char* mypwd;
    strcpy(mypwd, row[0]);

    // parse  the last filenanme
    char** cmd;
    int Count = myLastWd(mypwd, cmd);
    int i = 0;
    char lastWd[BUFFERSIZE] = {0};
    while (cmd[i] != NULL) {
        bzero(lastWd, BUFFERSIZE);
        strcpy(lastWd, cmd[i++]);
        // puts(cmd[i++]);
    }
    puts(lastWd);

    mysql_free_result(result);

    return 0;
}

/*传入传出参数 mypwd 为当前路径，成功返回0，失败返回-1*/
int myPwd(MYSQL* conn, const int userId, char* mypwd) {
    int ret;
    char query[BUFFERSIZE] = {0};
    sprintf(query,
            "select u_pwd from UserInfo where u_id = '%d';", userId);

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

/*成功返回 parent_id ( non-negative number )，失败返回 -1*/
int myMkdir(MYSQL* conn, const int userId, char* dirName) {
    int fileId;
    fileId = getPwdId(conn, userId);
    if (fileId == -1) {
        printf("error: getPwdId \n");
        return -1;
    }

    // mkdir
    int parentId = fileId;
    char query[BUFFERSIZE] = {0};
    sprintf(query,
            "insert into FileTable(parent_id, f_name, owner_id, f_md5, f_size, f_type) "
            "values('%d', '%s', '%d', '%s', '%d' );",
            parentId, dirName, userId, "0", 0);

    int ret = mysql_query(conn, query);
    if (ret != 0) {
        printf("error query1: %s\n", mysql_error(conn));
        return -1;
    }

    return parentId;
}
