#include <gxgfunc.h>
#define BUFFERSIZE 1024
#define TIMEOUT 30

typedef struct clientBox_s {
    int netFd;
    char userName[BUFFERSIZE];
    int userId;
    int slotIndex;
} clientBox_t;

typedef struct slotNode_s {
    int userId;
    int netFd;
    struct slotNode_s* next;
} slotNode_t;

typedef struct slotList_s {
    slotNode_t* head;
    slotNode_t* tail;
    int size;
} slotList_t;

int initSlotMap(slotList_t** slotMap, int timeOut);
int delFromSlot(int timeIndex, slotList_t** slotMap);
int removeFromSlot(int userId, int netFd, int slotIndex, slotList_t** slotMap);
int attachToSlot(int userId, int netFd, int slotIndex, slotList_t** slotMap);


int initSlotMap(slotList_t** slotMap, int timeOut) {
    for (int i = 0; i < timeOut; i++) {
        slotMap[i] = (slotList_t*)calloc(1, sizeof(slotList_t));
        slotMap[i]->head = NULL;
        slotMap[i]->tail = NULL;
        slotMap[i]->size = 0;
    }
}

/*将该timeIndex下的所有slotNode对应的netFd关闭*/
int delFromSlot(int timeIndex, slotList_t** slotMap) {
    slotNode_t* curr = slotMap[timeIndex]->head;
    while (curr != NULL) {
        slotNode_t* next = curr->next;  // 保存curr后继结点
        close(curr->netFd);             //关闭当前netFd
        free(curr);                     //释放空间
        curr = next;
    }
}

/*更新当前client的slotIndex时也要将该client对应的slotNode挂到timeIndex之下，成功赶回-1，失败返回0*/
int removeFromSlot(int userId, int netFd, int slotIndex, slotList_t** slotMap) {
    //删除slotMap[oldIndex]中userId相等的节点
    slotNode_t* prev = NULL;
    slotNode_t* curr = slotMap[slotIndex]->head;
    while (curr != NULL && curr->userId != userId) {
        prev = curr;
        curr = curr->next;
    }
    // 若没有这样的元素
    if (curr == NULL) {
        return -1;
    }
    //删除第一个元素
    if (prev == NULL) {
        if (slotMap[slotIndex]->size == 1) {  //如果只有一个元素
            slotMap[slotIndex]->head = slotMap[slotIndex]->tail = NULL;
        } else {
            slotMap[slotIndex]->head = curr->next;
        }
        free(curr);
    } else {
        prev->next = curr->next;
        if (prev->next == NULL) {//如果要删除尾节点
            slotMap[slotIndex]->tail = prev;
        }
        free(curr);
    }
    --(slotMap[slotIndex]->size);
    return 0;
}

/*在slotIndex处挂上slotNode，采用尾插法，成功返回0，失败返回0*/
int attachToSlot(int userId, int netFd, int slotIndex, slotList_t** slotMap) {
    slotNode_t* newSlotNode = (slotNode_t*)malloc(sizeof(slotNode_t));
    if (newSlotNode == NULL) {
        printf("Error: malloc failed in add_before_head.\n");
        return -1;
    }

    //初始化节点
    newSlotNode->userId = userId;
    newSlotNode->netFd = netFd;

    //判断链表是否为空
    if (slotMap[slotIndex]->size == 0) {  //若为空
        slotMap[slotIndex]->head = newSlotNode;
        slotMap[slotIndex]->tail = newSlotNode;
    } else {  //非空
        slotMap[slotIndex]->tail->next = newSlotNode;
        slotMap[slotIndex]->tail = newSlotNode;
    }
    ++(slotMap[slotIndex]->size);
    return 0;
}

int main() {
    int timeIndex = 0;
    slotList_t* slotMap[TIMEOUT];
    initSlotMap(slotMap, TIMEOUT);

    return 0;
}