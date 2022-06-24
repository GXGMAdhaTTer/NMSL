#define _GNU_SOURCE

#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <pthread.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/select.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/uio.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <shadow.h>
#include <errno.h>
#include <ctype.h>
#include <mysql/mysql.h>
// #include <openssl/md5.h>



#define ARGS_CHECK(argc, num)                 \
    {                                         \
        if (argc != num) {                    \
            fprintf(stderr, "args error!\n"); \
            return -1;                        \
        }                                     \
    }

#define ERROR_CHECK(ret, num, msg) \
    {                              \
        if (ret == num) {          \
            perror(msg);           \
            return -1;             \
        }                          \
    }

#define THREAD_ERROR_CHECK(ret, msg)                        \
    {                                                       \
        if (ret != 0) {                                     \
            fprintf(stderr, "%s:%s\n", msg, strerror(ret)); \
        }                                                   \
    }
