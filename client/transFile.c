#include "client.h"

void itoa(int n, char* s) {
    char buf[40] = {0};
    int i=0;
    while (n > 0) {
        buf[i++] = n % 10 + '0';
        n = n / 10;
    }
    for (int j = 0; j < i; j++) {
        s[j] = buf[i - j - 1];
    }
}

int sendn(int sockFd, void* pstart, int len) {
    int total = 0;
    int ret = 0;
    char* p = (char*)pstart;

    while (total < len) {
        ret = send(sockFd, p + total, len - total, MSG_NOSIGNAL);
        if(ret == -1){
            printf("连接已断开,请重新连接!\n");
            exit(0);
        }
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
        if(ret == 0){
            printf("连接已断开,请重新连接!\n");
            exit(0);
        }
        total += ret;
    }
    return 0;
}

int recvFile(int sockFd, char* fileName) {
    int total;

    // 尝试打开fileName.temp，看是否存在
    char tempFileName[BUFFERSIZE] = {0};
    sprintf(tempFileName, "%s.temp", fileName);
    int tempFd = open(tempFileName, O_RDWR);

    if (tempFd == -1) {  //若不存在temp文件，即便有fileName文件也需要重新下载并覆盖
        total = 0;
        tempFd = open(tempFileName, O_RDWR | O_CREAT);  //创建temp文件
    } else {                                            //若已存在temp文件
        //从temp中读取total
        char tempBuf[40] = {0};
        read(tempFd, tempBuf, sizeof(tempBuf));
        puts(tempBuf);
        total = atoi(tempBuf);
    }

    //发送total
    sendn(sockFd, &total, sizeof(int));

    int dataLength;

    int fd = open(fileName, O_RDWR | O_CREAT | O_TRUNC, 0666);
    ERROR_CHECK(fd, -1, "open");

    int fileSize;

    recvn(sockFd, &dataLength, sizeof(int));
    recvn(sockFd, &fileSize, dataLength);
    printf("fileSize = %d\n", fileSize);
    int doneSize = total;
    int lastSize = 0;
    int slice = fileSize / 10000;
    ftruncate(fd, fileSize);

    char* p = (char*)mmap(NULL, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ERROR_CHECK(p, MAP_FAILED, "mmap");
    //映射pTotal
    time_t timeBeg, timeEnd;

    timeBeg = time(NULL);

    while (total < fileSize) {
        recvn(sockFd, &dataLength, sizeof(int));
        if (dataLength != 1000) {
            printf("dataLength = %d\n", dataLength);
            printf("100.00%%\n");
            remove(tempFileName);
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

        //写入total
        char tempBuf[40] = {0};
        itoa(total, tempBuf);
        lseek(tempFd, 0, SEEK_SET);
        write(tempFd, tempBuf, strlen(tempBuf));

        usleep(sleepTime);
    }

    recvn(sockFd, p + total, dataLength);

    timeEnd = time(NULL);

    printf("totalTime = %lds\n", timeEnd - timeBeg);

    close(fd);
    munmap(p, fileSize);
}

int sendFile(const int netFd, char* fileName) {
    int fd = open(fileName, O_RDWR);
    ERROR_CHECK(fd, -1, "open");
    if (fd == -1) {
        return 0;
    }

    train_t train;
    train.length = strlen(fileName);
    strcpy(train.buf, fileName);

    int ret = send(netFd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);
    ERROR_CHECK(ret, -1, "send");
    if (ret == -1) {
        return 0;
    }

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
    int total = 0;

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