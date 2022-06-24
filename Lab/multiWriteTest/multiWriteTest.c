#include <gxgfunc.h>
typedef struct {
    int total;
    char filename[1024];
} args;

void* func(void* wtf) {
    printf("到这里了么？\n");
    args* whatthehell = (args*)wtf;
    printf("total=%d\n", whatthehell->total);
    char buf[1024] = {0};
    strcpy(buf, whatthehell->filename);
    int fd = open(whatthehell->filename, O_RDWR);

    char* p;
    p = (char*)mmap(NULL, 100, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    for (int i = 0; i < 25; ++i) {
        p[i + whatthehell->total] = 'B' + ((whatthehell->total) / 25);
    }
    munmap(p, 100);
    close(fd);
    printf("whatthehell %d done!\n", whatthehell->total);
}

int main() {
    int fd = open("testdownload.txt", O_RDWR | O_TRUNC | O_CREAT, 0666);
    ftruncate(fd, 100);
    char* p;

    p = (char*)mmap(NULL, 100, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    int i = 0;
    for (; i < 100; ++i) {
        p[i] = 'A';
    }

    munmap(p, 100);
    close(fd);

    args example[4] = {0};

    for (int j = 0; j < 4; ++j) {
        example[j].total = 25 * j;
        strcpy(example[j].filename, "testdownload.txt");
    }
    printf("example[0]=%d\n", example[0].total);
    printf("example[1]=%d\n", example[1].total);
    printf("example[2]=%d\n", example[2].total);
    printf("example[3]=%d\n", example[3].total);
    printf("example[0]=%s\n", example[0].filename);
    printf("example[1]=%s\n", example[1].filename);
    printf("example[2]=%s\n", example[2].filename);
    printf("example[3]=%s\n", example[3].filename);

    pthread_t ntid[4];
    for (int i = 0; i < 4; i++) {
        pthread_create(&ntid[i], NULL, func, (void*)&example[i]);
    }

    for (i = 0; i < 4; i++) {
        pthread_join(ntid[i], NULL);
    }
    return 0;
}