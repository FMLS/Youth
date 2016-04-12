#include <sys/select.h>
#include "youth.h"
#include "list.h"


typedef struct __yoListNode
{
    struct __yoListNode *next, *prev;
    int fd;
    int fdtype;
}yoListNode;


typedef struct __yoSelectReactor
{
    fd_set read_fds;
    yoListNode *fds; //链表结构
    int maxfd;
    int fd_num;
}yoSelectReactor;

void yoSelectReactor_free(yoReactor *reactor);
int yoSelectReactor_add(yoReactor *reactor, int fd, int fdtype);
int yoSelectReactor_cmp(yoListNode *a, yoListNode *b);
int yoSelectReactor_del(yoReactor *reactor, int fd);
int yoSelectReactor_wait(yoReactor *reactor, struct timeval *timeo);



int yoSelectReactor_create(yoReactor *reactor)
{
    yoSelectReactor * this = yo_malloc(sizeof(yoSelectReactor));
    if (this == NULL) {
        yoTrace("[yoSelectReactor_create] malloc failed \n");
        return YO_ERR;
    }
    this->fds = NULL;
    this->maxfd = 0;
    this->fd_num = 0;
    bzero(reactor->handle, sizeof(reactor->handle));

    reactor->object = this;
    reactor->add    = yoSelectReactor_add;
    reactor->del    = yoSelectReactor_del;
    reactor->free   = yoSelectReactor_free;
    reactor->wait   = yoSelectReactor_wait;
    reactor->setHandle = yoReactor_setHandle;
    return YO_OK;
}

void yoSelectReactor_free(yoReactor *reactor)
{
    yoListNode *ev;
    yoSelectReactor *this = reactor->object;
    LL_FOREACH(this->fds, ev)
    {
        LL_DELETE(this->fds, ev);
        yo_free(ev);
    }
    yo_free(reactor->object);
}


int yoSelectReactor_add(yoReactor *reactor, int fd, int fdtype)
{
    yoSelectReactor * this = reactor->object;
    yoListNode *ev = yo_malloc(sizeof(yoListNode));
    ev->fd = fd;
    ev->fdtype = fdtype;
    LL_APPEND(this->fds, ev);
    this->fd_num++;
    if (fd > this->maxfd) {
        this->maxfd = fd;
    }
    return YO_OK;
}

int yoSelectReactor_cmp(yoListNode *a, yoListNode *b)
{
    return (a->fd == b->fd) ? 0 :(a->fd > b->fd ? -1 : 1);
}

int yoSelectReactor_del(yoReactor *reactor, int fd)
{
    yoSelectReactor *this = reactor->object;
    yoListNode ev, *del;
    ev.fd = fd;
    LL_SEARCH(this->fds, del, &ev, yoSelectReactor_cmp); //del保存和ev.fd相等节点
    LL_DELETE(this->fds, del);
    this->fd_num--;
    yo_free(del);
    return YO_OK;
}

int yoSelectReactor_wait(yoReactor *reactor, struct timeval *timeo)
{
    yoSelectReactor *this = reactor->object;
    yoListNode *ev;
    yoEvent event;
    int ret;
    struct timeval timeout;

    while (youth_running) {
        FD_ZERO(&(this->read_fds));
        timeout.tv_sec = timeo->tv_sec;
        timeout.tv_usec = timeo->tv_usec;
        LL_FOREACH(this->fds, ev) {
            FD_SET(ev->fd, &(this->read_fds));
        }
        ret = select(this->maxfd + 1, &(this->read_fds), NULL, NULL, &timeout);
        if (ret < 0) {
            yoTrace("select error, Errno: %d\n", errno);
            if (errno == EINTR) {
                continue;
            }
            else {
                return YO_ERR;
            }
        }
        else if (ret == 0) {
            continue;
        }
        else {
            LL_FOREACH(this->fds, ev) {
                if (FD_ISSET(ev->fd, &(this->read_fds))) {
                    event.fd = ev->fd;
                    event.from_id = reactor->id;
                    event.type = ev->fdtype;
                    yoTrace("Event:Handle=%p | fd= %d | from_id= %d | type= %d\n",reactor->handle[event.type], ev->fd, reactor->id, ev->fdtype);
                    reactor->handle[event.type](reactor, &event);
                }
            }
        }
    }
    return 0;
}

int main()
{
    yoReactor reactor;
    yoSelectReactor_create(&reactor);




    return 0;
}
