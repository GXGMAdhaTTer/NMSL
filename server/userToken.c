#include "server.h"
/*向 UserToken表中添加 user_token 记录, 成功返回0, 失败返回-1*/
int InserIntoUserToken(MYSQL* conn, const int userId, const char* userToken) {
    struct tm curTime;
    time_t now;
    time(&now);
    localtime_r(&now, &curTime);

    char query[1024] = {0};
    sprintf(query,
            "insert into UserToken(u_id, t_token, t_expire) "
            "values( %d, '%s', '%04d-%02d-%02d %02d:%02d:%02d');",
            userId, userToken, curTime.tm_year + 1900, curTime.tm_mon + 1,
            curTime.tm_mday, curTime.tm_hour, curTime.tm_min, curTime.tm_sec);

    int ret = mysql_query(conn, query);
    if (ret != 0) {
        printf("error query1: %s\n", mysql_error(conn));
        return -1;
    }
    return 0;
}

/*用户退出时删除该用户的 token记录, 成功返回0, 失败返回-1*/
int delFromUserToken(MYSQL* conn, const int userId) {
    struct tm t;
    time_t now;
    time(&now);
    localtime_r(&now, &t);

    char query[1024] = {0};
    sprintf(query,
            "delete from UserToken "
            "where u_id = %d; ",
            userId);

    int ret = mysql_query(conn, query);
    if (ret != 0) {
        printf("error query1: %s\n", mysql_error(conn));
        return -1;
    }
    return 0;
}

/*每次调用时，将该用户Token记录的 t_expire + EXPIRETIME, 成功返回0, 失败返回-1*/
int expireUserToken(MYSQL* conn, const int userId) {
    int ret;
    char query[1024] = {0};
    
    char buf[1024] = {0};

    struct tm t;
    time_t now;
    time(&now);
    localtime_r(&now, &t);


    int year = t.tm_year + 1900;
    int month = t.tm_mon + 1;
    int day = t.tm_mday;
    int hour = t.tm_hour;
    int minute = t.tm_min;
    int second = t.tm_sec;

    //刷新时间
    for (int i = 0; i < EXPIRETIME; i++) {
        char monthDays[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (0 == (year % 4 && 0 != year % 100) || 0 == year % 400)
            monthDays[2] = 29;
        if (59 == second) {
            minute += 1;
            second = 0;
            if (60 == minute) {
                hour += 1;
                minute = 0;
                if (24 == hour) {
                    day += 1;
                    hour = 0;
                    if (day > monthDays[month]) {
                        month += 1;
                        day = 1;
                        if (13 == month) {
                            year += 1;
                            month = 1;
                        }
                    }
                }
            }

        } else {
            second += 1;
        }
    }

    char newExpire[1024] = {0};
    sprintf(newExpire, "%d-%d-%d %d:%d:%d", year, month, day, hour, minute,
            second);

    sprintf(query,
            "update UserToken set t_expire = '%s' "
            "where u_id = %d;",
            newExpire, userId);

    ret = mysql_query(conn, query);
    if (ret != 0) {
        printf("error query3: %s\n", mysql_error(conn));
        return -1;
    }

    return 0;
}