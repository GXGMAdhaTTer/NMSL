#include "gxgfunc.h"
#include "md5.h"
#define CLIENTMAX 100
#define EPOLLNUM 1000
#define THREADNUM 3
#define IPADDRESS "10.211.55.5"
#define MAINPORT 1234
#define SUBPORT 1688
#define BUFFERSIZE 1024
#define ARGCMAX 10
#define SALTSIZE 8
#define ROUTEDEPTH 10
#define EXPIRETIME 30
#define TIMEOUT 10

/*用于生成Token的KEY*/
static const char KEY[] = "NetdiskManagementServiceLeague";

/*文件类型*/
enum FILETYPE {
    DIRFILE,
    COMMONFILE
};

/*返回类型*/
enum RETURNCODE {
    SUCCESS = 0,
    FAILED = -1
};

/*任务类型，GETS为0，PUTS为1*/
enum TASKTYPE {
    GETS,
    PUTS
};

/*任务结构体*/
typedef struct task_s {
    int netFd;  //传递文件描述符
    // int taskType;               //传递任务类型
    MYSQL* conn;
    char userName[BUFFERSIZE];
    struct task_s* pNext;
} task_t;

/*任务队列链表*/
typedef struct taskQueue_s {
    task_t* pFront;  //队首
    task_t* pRear;   //队尾
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} taskQueue_t;

/*线程池*/
typedef struct threadPool_s {
    pthread_t* tid;  //子线程的数组
    int threadNum;   //子线程数量
    taskQueue_t taskQueue;
    int exitFlag;
} threadPool_t;

/*用户信息结构体*/
typedef struct clientBox_s {
    int netFd;
    char userName[BUFFERSIZE];
    int userId;
    int slotIndex;
} clientBox_t;

/*文件协议*/
typedef struct train_s {
    int length;
    char buf[1000];
} train_t;

/*信息结构体*/
typedef struct msgBox_s {
    int length;
    char buf[BUFFERSIZE];
} msgBox_t;

/*文件信息结构体*/
typedef struct fileInfo_s {
    int parentId;
    char fileName[BUFFERSIZE];
    int ownerId;
    char md5[40];
    int fileSize;
    int fileType;
} fileInfo_t;

/*slot节点*/
typedef struct slotNode_s {
    int userId;
    int netFd;
    struct slotNode_s* next;
} slotNode_t;

/*环形队列slot链表*/
typedef struct slotList_s {
    slotNode_t* head;
    slotNode_t* tail;
    int size;
} slotList_t;

//线程池
int taskEnqueue(taskQueue_t* pTaskQueue, int netFd, MYSQL* conn, char* userName);
int taskDequeue(taskQueue_t* pTaskQueue);
int threadPoolInit(threadPool_t* pThreadPool, int workerNum);
int makeWorker(threadPool_t* pThreadPool);
int epollAdd(int fd, int epfd);
int epollDel(int fd, int epfd);

//网络传输
int tcpInit(int* pSockFd, char* ip, int port);
int sendn(int sockFd, void* pstart, int len);
int recvn(int sockFd, void* pstart, int len);
int recvFile(int sockFd, char* fileName);
int sendFile(int netFd, char* fileName);

// MYSQL
MYSQL* mySqlInit();

//用户表
int isUser(MYSQL* conn, const char* userName, char* salt, char* cryptpasswd);
int getUserId(MYSQL* conn, const char* userName);
void getRandStr(char* mystr);
int addUser(MYSQL* conn, const char* userName, const char* userPasswd);

//文件表
int addHomeDir(MYSQL* conn, char* userName);
int initUserPwd(MYSQL* conn, char* userName);
int updateUserPwd(MYSQL* conn, const char* userName, char* curPwd);
int myLastWd(char* buf, char* lines[]);
int getPwdFId(MYSQL* conn, const int userId);
int getFileInfo(MYSQL* conn, const int userId, const char* fileName, const int fileType, const int parent_id, char* md5);
int isFileExistInPwd(MYSQL* conn, const char* fileName, int userId, int curF_id, int fileType);
int isFileExistInTable(MYSQL* conn, const char* fileName, char* md5);
int DelFileFromTable(MYSQL* conn, const int userId, const char* fileName, const int fileType, const int parent_id);
int addFileToTable(MYSQL* conn, const int userId, const char* fileName, const char* md5, const int fileType, const int parent_id);

//命令
int clientIntro(int netFd, MYSQL* conn, char* userName);
int vFinder(int netFd, MYSQL* conn, char* userName, threadPool_t* pThreadPool);
int myParse(char* buf, char* cmd[]);
int myPwd(MYSQL* conn, const int userId, char* mypwd);
int myLs(MYSQL* conn, const int curF_id, const int userId, char* Ls);
int myMkdir(MYSQL* conn, const int userId, char* dirName);
int myCd(MYSQL* conn, char* fileName, const char* userName, char* curPwd);
int myRm(MYSQL* conn, const int userId, char* fileName);
int myRmdir(MYSQL* conn, const int userId, char* dirName);

//Token表
int encodeToken(char* userName, char* Token);
int decodeToken(char* userName, const char* JWT);
int InserIntoUserToken(MYSQL* conn, const int userId, const char* userToken);
int delFromUserToken(MYSQL* conn, const int userId);
int expireUserToken(MYSQL* conn, const int userId);

//日志表
int addToLog(MYSQL* conn, const int user_id, const char* user_name, const char* op_name);

//环形队列
int initSlotMap(slotList_t** slotMap, int timeOut);
int clearSlot(int timeIndex, slotList_t** slotMap, clientBox_t* clients, int* pClientNum);
int removeFromSlot(int userId, int netFd, int slotIndex, slotList_t** slotMap);
int attachToSlot(int userId, int netFd, int slotIndex, slotList_t** slotMap);
int initClients(clientBox_t* clients, int* clientNum);
int addToClients(clientBox_t* clients, int netFd, char* userName, int userId, int timeIndex, slotList_t** slotMap, int epfd, int* pClientNum);
int delFromClients(clientBox_t* clients, int userId);