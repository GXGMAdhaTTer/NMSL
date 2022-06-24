#include "client.h"

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