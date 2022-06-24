#include "server.h"
int sendn(int sockFd, void* pstart, int len) {
    int total = 0;
    int ret = 0;
    char* p = (char*)pstart;

    while (total < len) {
        ret = send(sockFd, p + total, len - total, MSG_NOSIGNAL);
        total += ret;
    }
    return 1;
}
int recvn(int sockFd, void* pstart, int len) {
    int total = 0;
    int ret;
    char* p = (char*)pstart;

    while (total < len) {
        ret = recv(sockFd, p + total, len - total, 0);
        total += ret;
    }
    return 0;
}

int recvFile(int sockFd, char* filename) {
    char name[1024] = {0};
    int dataLength;

    int ret = recvn(sockFd, &dataLength, sizeof(int));

    recvn(sockFd, name, dataLength);
    puts(name);

    int fd = open(name, O_RDWR | O_CREAT | O_TRUNC, 0666);
    ERROR_CHECK(fd, -1, "open");

    int fileSize;

    recvn(sockFd, &dataLength, sizeof(int));
    recvn(sockFd, &fileSize, dataLength);
    printf("fileSize = %d\n", fileSize);
    int doneSize = 0;
    int lastSize = 0;
    int slice = fileSize / 10000;
    ftruncate(fd, fileSize);

    char* p =
        (char*)mmap(NULL, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ERROR_CHECK(p, MAP_FAILED, "mmap");
    int total = 0;

    time_t timeBeg, timeEnd;

    timeBeg = time(NULL);

    while (total < fileSize) {
        recvn(sockFd, &dataLength, sizeof(int));
        if (dataLength != 1000) {
            printf("dataLength = %d\n", dataLength);
            printf("100.00%%\n");
            break;
        }
        doneSize += dataLength;
        if (doneSize - lastSize > slice) {
            printf("%5.2lf%%\r", 100.0 * doneSize / fileSize);
            fflush(stdout);
            lastSize = doneSize;
        }

        recvn(sockFd, p + total, dataLength);
        total += dataLength;
        // usleep(2000);
    }

    recvn(sockFd, p + total, dataLength);

    timeEnd = time(NULL);

    printf("totalTime = %lds\n", timeEnd - timeBeg);

    close(fd);
    munmap(p, fileSize);
}

int sendFile(const int netFd, char* fileName) {
    int ret;

    int fd = open(fileName, O_RDWR);
    ERROR_CHECK(fd, -1, "open");
    if (fd == -1) {
        return 0;
    }

    train_t train;

    //接收total
    int total = 0;
    recvn(netFd, &total, sizeof(int));
    printf("temptotal = %d\n", total);

    //发送文件总大小
    train.length = 4;
    struct stat statbuf;
    ret = fstat(fd, &statbuf);
    int fileSize = statbuf.st_size;
    memcpy(train.buf, &fileSize, sizeof(int));
    send(netFd, &train, train.length + sizeof(train.length), MSG_NOSIGNAL);

    char* p = (char*)mmap(NULL, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ERROR_CHECK(p, MAP_FAILED, "mmap");
    if (ret == -1) {
        return 0;
    }

    while (total < fileSize) {
        if (fileSize - total < sizeof(train.buf)) {  //不足1000B
            train.length = fileSize - total;
        } else {
            train.length = sizeof(train.buf);
        }
        memcpy(train.buf, p + total, train.length);
        total += train.length;
        send(netFd, &train, train.length + sizeof(train.length), MSG_NOSIGNAL);
    }

    close(fd);
    munmap(p, fileSize);
    return 1;
}