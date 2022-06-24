#include<gxgfunc.h>
#define BUFFERSIZE 1024

/*向日志文件表中添加日志记录, 成功返回1, 失败返回0*/
int addFileTologTable(MYSQL* conn, const int log_id, const int user_id , const char* user_name, const char* op_name) {
    int ret;

    struct tm t;
    time_t now;
    time(&now);
    localtime_r(&now,&t);

    char query[BUFFERSIZE] = {0};
    sprintf(query,
            "insert into log(log_id, user_id, user_name, op_name, op_time) "
            "values('%d', '%d', '%s', '%s', '%04d-%02d-%02d %02d:%02d:%02d');",
            log_id, user_id,user_name, op_name, t.tm_year+1900,t.tm_mon+1,t.tm_mday,t.tm_hour,t.tm_min,t.tm_sec);

    ret = mysql_query(conn, query);
    if (ret != 0) {
        printf("error query1: %s\n", mysql_error(conn));
        return 0;
    }
    return 1;
}