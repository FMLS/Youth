#include <sys/select.h>
#include "youth.h"
//#include "list.h"


typedef struct __yoListNode
{
    struct __yoListNode *next, *prev;
    int fd;
    int fdtype;
}yoListNode;


typedef struct __yoSelectReactor
{
    fd_set rfds;
    yoListNode *fds;
    int maxfd;
    int fd_num;
}yoSelectReactor;

int yoSelectReactor_create(yoReactor *reactor)
{
    return 0;
}

void yoSelectReactor_free(yoReactor *reactor)
{

}


int yoSelectReactor_add(yoReactor *reactor, int fd, int fdtype)
{
    return 0;
}

int yoSelectReactor_cmp(yoListNode *a, yoListNode *b)
{
    return (a->fd == b->fd) ? 0 :(a->fd > b->fd ? -1 : 1);
}

int yoSelectReactor_del(yoReactor *reactor, int fd)
{
    return 0;
}

int yoSelectReactor_wait(yoReactor *reactor, struct timeval *timeo)
{
    return 0;
}

int main()
{
    return 0;
}
