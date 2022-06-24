#include <gxgfunc.h>

#define BUFFERSIZE 1024

/*返回指令个数，传入传出参数是cmd指针数组*/
int myLastWd(char* buf, char* cmd[]);

/*mysql初始化连接*/
MYSQL* mySqlInit();

/*传入传出参数 mypwd 为当前路径，成功返回0，失败返回-1*/
int myPwd(MYSQL* conn, const int userId, char* mypwd);

/*传入传出参数 lastWord 为 pwd最后一个文件夹，成功返回0，失败返回-1*/
int getPwdLast(MYSQL* conn, const int userId, char* lastWord);

/*这个接口 用来 mkdir p_id，不允许一个用户新建已经使用过的文件夹名称*/
/*成功返回 f_id ( non-negative number )，失败返回 -1*/
int getPwdId(MYSQL* conn, const int userId);

/*这个接口不知道有什么用处，不允许一个用户新建已经使用过的文件夹名称*/
/*成功返回 parent_id ( non-negative number )，失败返回 -1*/
int getParentId(MYSQL* conn, const int userId);

/*传入传出参数 mypwd 为当前路径，成功返回0，失败返回-1*/
int myPwd(MYSQL* conn, const int userId, char* mypwd);


/*成功返回 parent_id ( non-negative number )，失败返回 -1*/
int myMkdir(MYSQL* conn, const int userId, char* dirName);