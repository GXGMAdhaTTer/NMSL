#include <gxgfunc.h>
#include "md5.h"

#define ROUTEDEPTH 10
#define BUFFERSIZE 1024
#define SALTSIZE 8

enum FILETYPE {
    DIRFILE,
    COMMONFILE
};

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

/*检查是否是用户，如果是用户则传送salt和cryptpasswd,并返回u_id*/
int isUser(MYSQL* conn, const char* userName, char* salt, char* cryptpasswd) {
    int ret;
    char query[BUFFERSIZE] = {0};
    sprintf(query,
            "select u_id, u_salt, u_cryptpasswd from UserInfo where u_name = '%s';",
            userName);

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
    strcpy(salt, row[1]);
    strcpy(cryptpasswd, row[2]);
    mysql_free_result(result);

    return userId;  //返回u_id
}

/*根据userName获取userId*/
int getUserId(MYSQL* conn, char* userName) {
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

/*添加用户，用于注册用户*/
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
        return 0;
    }
    return 1;
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
    if (fileType == 1) {  //如果是普通文件则返回md5，如果是文件夹则md5戊戌返回
        strcpy(md5, row[1]);
    }
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

void itoa(int n, char* s) {
    char buf[40] = {0};
    int i = 0;
    while (n > 0) {
        buf[i++] = n % 10 + '0';
        n = n / 10;
    }
    for (int j = 0; j < i; j++) {
        s[j] = buf[i - j - 1];
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

int main() {
    // char userName[BUFFERSIZE] = {0};

    //连接MYSQL
    MYSQL* conn = mySqlInit();
    ERROR_CHECK(conn, NULL, "mySqlInit");
    int userId = getUserId(conn, "gxg");
    

    int ret = isFileExistInPwd(conn, "file1.txt", userId, 1, COMMONFILE);
    printf("ret = %d\n", ret);
    // int ret = DelFileFromTable(conn, userId, "gxgsdir2", DIRFILE, 1);
    // int ret = DelFileFromTable(conn, userId, "gxgnmb", COMMONFILE, 1);
    // if (ret == 0) {
    //     printf("删除成功！\n");
    // }

    // char md5[BUFFERSIZE] = {0};
    // Compute_file_md5("file1.txt", md5);
    // struct stat statBuf;
    // stat("file1.txt", &statBuf);
    // int fileType;
    // if(S_ISDIR(statBuf.st_mode)){
    //     fileType = DIRFILE;
    // } else if(S_ISREG(statBuf.st_mode)){
    //     fileType = COMMONFILE;
    // }
    // addFileToTable(conn, userId, "file1.txt", md5, fileType, 1);

    // hjt 990624
    // scanf("%s", userName);

    mysql_close(conn);

    return 0;
}