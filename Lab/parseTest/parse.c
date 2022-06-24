#include <gxgfunc.h>

#define ARGCMAX 10
#define BUFFERSIZE 1024

/*返回指令个数，传入传出参数是cmd指针数组*/
int myParse(char* buf, char* cmd[]) {
    int argc = 0;
    char* word;
    if (buf[strlen(buf) - 1] == '\n') {
        buf[strlen(buf) - 1] = '\0';
    }

    char tempBuf[BUFFERSIZE] = {0};
    strcpy(tempBuf, buf);

    word = strtok(tempBuf, " ");
    cmd[0] = word;
    int i = 1;
    while ((word = strtok(NULL, " ")) != NULL && i < ARGCMAX) {
        cmd[i++] = word;
    }
    cmd[i] = NULL;
    argc = i;
    return argc;
}

/*返回指令个数，传入传出参数是cmd指针数组*/
int myLastWd(char* buf, char* cmd[]) {
    int argc = 0;
    char* word;
    if (buf[strlen(buf) - 1] == '\n') {
        buf[strlen(buf) - 1] = '\0';
    }

    char tempBuf[BUFFERSIZE] = {0};
    strcpy(tempBuf, buf);

    word = strtok(tempBuf, "/");
    cmd[0] = word;
    int i = 1;
    while ((word = strtok(NULL, "/")) != NULL && i < ARGCMAX) {
        cmd[i++] = word;
    }
    cmd[i] = NULL;
    argc = i;
    return argc;
}

int main() {
    // char buf[] = "login file fqw";
    char buf[] = "/gxg/";
    char* cmd[ARGCMAX] = {0};
    // int cmdCount = myParse(buf, cmd);
    // printf("cmd = %d\n", cmdCount);
    // int i = 0;
    // while (cmd[i] != NULL) {
    //     puts(cmd[i++]);
    // }

    int cmdCount = myLastWd(buf, cmd);
    printf("cmd = %d\n", cmdCount);
    int i = 0;
    char lastWd[BUFFERSIZE] = {0};
    while (cmd[i] != NULL) {
        bzero(lastWd, BUFFERSIZE);
        strcpy(lastWd, cmd[i++]);
        // puts(cmd[i++]);
    }
    puts(lastWd);

    return 0;
}