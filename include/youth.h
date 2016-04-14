#ifndef YOUTH_H_
#define YOUTH_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
//Linux系统调用头文件
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/select.h>
#include <pthread.h>

#define YO_MAX_FDS      (1024 * 10) //最大文件描述符数量
#define YO_THREAD_NUM   2           //线程数量
#define YO_WRITER_NUM   2           //写线程的数量
#define YO_TASK_THREAD  4
#define YO_WORKER_NUM   4           //worker进程的数量
#define YO_PIPES_NUM    (YO_WORKER_NUM/YO_WRITER_NUM + 1) //每个写线程pipes数组大小
#define YO_BUFFER_SIZE  65495 //udp最大包(65535) - 包头(28)- 3*int
#define YO_BACKLOG      512
#define YO_TIMEO_SEC    0
#define YO_TIMEO_USEC   3000000

#ifndef MAX
#define MAX(a, b)       (a) > (b) ? a : b;
#endif

#define YO_START_SLEEP  usleep(1000 * 10)

#ifdef YO_USE_PHP
#define yo_malloc   emalloc
#define yo_free     efree
#define yo_calloc   ecalloc
#define yo_realloc  erealloc
#else
#define yo_malloc   malloc
#define yo_free     free
#define yo_calloc   calloc  
#define yo_realloc  realloc
#endif

//统一返回值
#define YO_OK           0
#define YO_ERR          -1

#define YO_FD_CONN      0
#define YO_FD_LISTEN    1
#define YO_FD_CLOSE     2
#define YO_FD_ERROR     3

#define YO_MODE_CALL    1
#define YO_MODE_THREAD  2
#define YO_MODE_PROCESS 3

#ifdef YO_DEBUG
#define yoTrace(str, ...)    {printf("[%s:%d:%s]"str,__FILE__,__LINE__,__func__,##__VA_ARGS__);}
#else
#define yoTrace(str, ...)   {snprintf(sw_error,YO_ERROR_MSG_SIZE,"[%s:%d:%s]"str,__FILE__,__LINE__,__func__,##__VA_ARGS__);}
#endif

#define YO_CPU_NUM      sysconf(_SC_NPROCESSORS_ONLN) //保留

#define YO_MAX_FDTYPE       32
#define YO_ERROR_MSG_SIZE   256






typedef struct __yoEventData
{
    int fd;
    int len;
    int from_id;
    char data[YO_BUFFER_SIZE];
}yoEventData;


typedef struct __yoSendData
{
    int fd;
    int len;
    char *data;
}yoSendData;

typedef struct __yoEvent
{
    int fd;
    int from_id;
    int type;
}yoEvent;

typedef struct __yoReactor yoReactor;
typedef int (*yoReactor_handle)(yoReactor *reactor, yoEvent *event);

typedef struct __yoFactory
{
    int  id;
    void *object;
    void *ptr;
    yoReactor *reactor;

    int (*start)    (struct __yoFactory *               );
    int (*shutdown) (struct __yoFactory *               );
    int (*dispatch) (struct __yoFactory *, yoEventData *);
    int (*finish)   (struct __yoFactory *, yoSendData  *);
    int (*onTask)   (struct __yoFactory *, yoEventData *);
    int (*onFinish) (struct __yoFactory *, yoSendData  *);
}yoFactory;

struct __yoReactor
{
    int id;
    void *ptr;
    void *object;
    yoFactory *factory;
    yoReactor_handle handle[YO_MAX_FDTYPE];

    int  (*add)      (yoReactor *, int, int);
    int  (*del)      (yoReactor *, int     );
    int  (*wait)     (yoReactor *, struct timeval *);
    void (*free)     (yoReactor *          );
    int  (*setHandle)(yoReactor *, int , yoReactor_handle);
};









#endif
