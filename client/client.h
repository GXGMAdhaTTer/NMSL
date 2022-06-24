#include "gxgfunc.h"
#include "md5.h"
#define THREADNUM 3
#define IPADDRESS "10.211.55.5"
#define MAINPORT 1234
#define SUBPORT 1688
#define BUFFERSIZE 1024
#define ARGCMAX 10
#define SALTSIZE 8
#define ROUTEDEPTH 10

int sleepTime;

/*文件类型*/
enum FILETYPE {
    DIRFILE,
    COMMONFILE
};

/*返回类型*/
enum RETURNCODE {
    SUCCESS,
    FAILED
};

/*任务类型，GETS为0，PUTS为1*/
enum TASKTYPE {
    GETS,
    PUTS
};

/*任务结构体*/
typedef struct task_s {
    int port;  //传递连接端口
    char userName[BUFFERSIZE]; //传递userName
    char userToken[BUFFERSIZE]; //传递userToken
    int taskType;               //传递任务类型
    char fileName[BUFFERSIZE];  //传递文件名
    int curF_id;
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

/*客户端box*/
typedef struct clientBox_s {
    int netFd;
    char userName[BUFFERSIZE];
    int userId;
} clientBox_t;

/*文件协议*/
typedef struct train_s {
    int length;
    char buf[BUFFERSIZE];
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

int connectInit(int* pSockFd, char* ip, int port);
int userIntro(const int sockFd, char* userName, int userNameLen);
int recvn(int sockFd, void* pstart, int len);
int sendn(int sockFd, void* pstart, int len);
int recvFile(int sockFd, char* fileName);
int sendFile(int netFd, char* fileName);
int newSendFile(int netFd, char* fileName);
int newRecvFile(int sockFd);
int vShell(int sockFd, char* userName, threadPool_t* pThreadPool);
int myParse(char* buf, char* cmd[]);
int epollAdd(int fd, int epfd);
int epollDel(int fd, int epfd);
int taskEnqueue(taskQueue_t* pTaskQueue, int port, char* userName, char* userToken, int taskType, char* fileName, int curF_id);
int taskDequeue(taskQueue_t* pTaskQueue);
int threadPoolInit(threadPool_t* pThreadPool, int workerNum);
void cleanFunc(void* arg);
void* handEvent(void* arg);
int makeWorker(threadPool_t* pThreadPool);