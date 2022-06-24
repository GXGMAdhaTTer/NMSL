#include"server.h"


int tcpInit(int* pSockFd, char* ip, int port){
    *pSockFd = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_CHECK(*pSockFd, -1, "socket");
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    int reuse = 1;
    int ret = setsockopt(*pSockFd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
    ERROR_CHECK(ret,-1,"setsockopt");
    ret = bind(*pSockFd,(struct sockaddr *)&addr, sizeof(addr));
    ERROR_CHECK(ret,-1,"bind");
    listen(*pSockFd,10);    
}