#include "myShtCmd.h"
int main() {
    //连接 MYSQL
    MYSQL* conn = mySqlInit();
    ERROR_CHECK(conn, NULL, "mySqlInit");

    // test pwd
    char userName[BUFFERSIZE] = {0};
    char mypwd[BUFFERSIZE] = {0};

    int ret = getPwdId(conn, 1);
    if(ret == -1)
    {
        printf("Error:getFileId\n");
        return -1;
    }

    ret = myPwd(conn, 1, mypwd);
    puts(mypwd);

    // mysql_free_result(result);

    mysql_close(conn);

    return 0;
}