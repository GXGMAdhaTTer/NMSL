#include <gxgfunc.h>
#include "md5.h"
#define BUFFERSIZE 1024

enum FILETYPE{
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

/*根据userName获取u_id，若未找到则返回0，发生错误则返回-1，可能是个没用的函数？*/
int getUserId(MYSQL* conn, const char* userName) {
    int userId = 0;
    int ret;
    char query[BUFFERSIZE] = {0};
    sprintf(query, "select u_id from UserInfo where u_name = '%s';", userName);

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

    //查不到此人，即 num_rows = 0
    if (mysql_num_rows(result) == 0) {
        return 0;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    userId = atoi(row[0]);

    return userId;
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

/*用于获取文件id和md5，通过用户、文件名、文件类型、父id来获取文件id和md5*/
int getFileInfo(MYSQL* conn,
                const int userId,
                const char* fileName,
                const int fileType,
                const int parent_id,
                char* md5) {
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
            return 0;
        }
        fstat(fd, &statBuf);
        f_size = statBuf.st_size;
        printf("file size %ld\n", statBuf.st_size);
    } else {
        printf("不支持本类型文件。\n");
        return 0;
    }

    char query[BUFFERSIZE] = {0};
    sprintf(query,
            "insert into FileTable(parent_id, f_name, owner_id, f_md5, f_size, f_type) "
            "values('%d', '%s', '%d', '%s', '%ld', '%d');",
            parent_id, fileName, userId, md5, f_size, fileType);

    ret = mysql_query(conn, query);
    if (ret != 0) {
        printf("error query1: %s\n", mysql_error(conn));
        return 0;
    }
    return 1;
}

/*删除文件表中的文件，默认当前工作目录位parent_id所指的文件夹，即已知parent_id*/
/*删除成功返回1，失败返回0*/
int DelFileFromTable(MYSQL* conn,
                     const int userId,
                     const char* fileName,
                     const char* md5,
                     const int fileType,
                     const int parent_id) {
    int ret;

    char query[BUFFERSIZE] = {0};

    int f_id = getFileInfo(conn, userId, fileName, 'd', parent_id, "0");

    if (fileType == DIRFILE) {  //是文件夹，如果存在，查看文件夹中是否有文件，若有则防止删除
        //判断文件夹下是否有文件
        sprintf(query, "select * from FileTable where parent_id = %d;", f_id);
        ret = mysql_query(conn, query);
        if (ret != 0) {
            printf("error query1: %s\n", mysql_error(conn));
            return 0;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (result == NULL) {
            printf("error query2: %s\n", mysql_error(conn));
            return -1;
        }

        int rowNums = mysql_num_rows(result);

        //如果文件夹中存在文件，直接返回0
        if (rowNums > 0) {
            printf("文件夹中有文件存在，删除失败！\n");
            return 0;
        }

        //如果文件夹中不存在文件，删除文件夹
        sprintf(
            query,
            "delete from FileTable "
            "where parent_id = %d "
            "and f_name = '%s' "
            "and owner_id = %d "
            "and f_md5 = '%s' "
            "and f_type = %d;",
            parent_id, fileName, userId, md5, fileType);

        ret = mysql_query(conn, query);
        if (ret != 0) {
            printf("error query1: %s\n", mysql_error(conn));
            return 0;
        }
    } else if (fileType == COMMONFILE) {  //如果是普通文件,直接该文件记录,删除文件
        sprintf(
            query,
            "delete from FileTable "
            "where parent_id = %d "
            "and f_name = '%s' "
            "and owner_id = %d "
            "and f_md5 = '%s' "
            "and f_type = %d;",
            parent_id, fileName, userId, md5, fileType);

        ret = mysql_query(conn, query);
        if (ret != 0) {
            printf("error query1: %s\n", mysql_error(conn));
            return 0;
        }
    }
    return 1;  //删除成功
}

int main() {
    //连接MYSQL
    MYSQL* conn = mySqlInit();
    ERROR_CHECK(conn, NULL, "mySqlInit");

    char fileName[BUFFERSIZE] = {0};
    char md5Val[BUFFERSIZE] = {0};
    int userId = 1;

    userId = getUserId(conn, "gxg");
    // printf("user_id = %d\n", userId);

    // strcpy(fileName, "gxgnmb");
    // Compute_file_md5(fileName, md5Val);

    // int f_id = 0;

    // if (f_id = isFileExistInTable(conn, fileName, md5Val)) {
    //     printf("文件存在。f_id = %d\n", f_id);
    // } else {
    //     printf("文件不存在。\n");
    // }

    //添加根目录文件夹，这个需要添加到注册用户中
    // strcpy(fileName, "gxg");
    // addFile(conn, userId, fileName, "0", 'd', 0);

    //添加测试文件
    // strcpy(fileName, "gxgnmb");
    // Compute_file_md5(fileName, md5Val);
    // addFileToTable(conn, userId, fileName, md5Val, 'f',
    //                isFileExistInTable(conn, "gxg", "0"));

    int parent_id = isFileExistInTable(conn, "gxg", "0");

    //测试获取文件id和md5
    int f_id = getFileInfo(conn, userId, "gxgsdir", DIRFILE, parent_id, md5Val);
    printf("%d\n", f_id);

    //添加文件夹
    // addFileToTable(conn, userId, "gxgsdir", "0", DIRFILE, parent_id);

    //删除文件夹
    // DelFileFromTable(conn, userId, "gxgsdir", "0", DIRFILE, parent_id);

    mysql_close(conn);
    return 0;
}