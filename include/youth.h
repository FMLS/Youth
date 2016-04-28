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
//#define YO_DEBUG 1

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
#define yoTrace(str, ...)   {snprintf(yo_error,YO_ERROR_MSG_SIZE,"[%s:%d:%s]"str,__FILE__,__LINE__,__func__,##__VA_ARGS__);}
#endif

#define YO_CPU_NUM      sysconf(_SC_NPROCESSORS_ONLN) //保留

#define YO_MAX_FDTYPE       32
#define YO_ERROR_MSG_SIZE   256

int youth_running;
int yo_errno;
char yo_error[YO_ERROR_MSG_SIZE];




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

typedef struct __yoEventClose
{
    int from_id;
    int fd;
}yoEventClose;

typedef struct __yoEventConnect
{
    int from_id;
    int conn_fd;
    int serv_fd;
    struct sockaddr_in addr;
    socklen_t addrlen;
}yoEventConnect;

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

typedef struct __yoWorker
{
    pid_t pid;                  //precess pid
    int pipe_fd;            
    int writer_id;
}yoWorker;

typedef struct __yoThreadWriter
{
    pthread_t ptid;             //thread id
    int event_fd;               
    int pipe_num;               //this thread's pipe num
    int *pipes_to_worker;       //pipe to worker process
    int c_pipe;                 //current pipe
    yoReactor reactor;          //thread's reactor
}yoThreadWriter;

typedef struct __yoFactoryProcess
{
    yoThreadWriter *writers;
    yoWorker       *workers;
    
    int writer_num;
    int worker_num;
    int c_writer_id; //current writer id
    int c_worker_id; //current worker id
}yoFactoryProcess;


typedef struct __yoThreadParam
{
    void *object;
    int  pti;
}yoThreadParam;

//yoReactor函数声明
int yoReactor_accept(yoReactor *reactor, yoEvent *event);
int yoReactor_close(yoReactor *reactor, yoEvent *event);
int yoReactor_setHandle(yoReactor *reactor, int fdtype, yoReactor_handle handle);
int yoReactor_receive(yoReactor *reactor, yoEvent *event);

//yoSelectReactor函数声明
int yoSelectReactor_create(yoReactor *reactor);
int yoSelectReactor_add(yoReactor *reactor, int fd, int fdtype);
int yoSelectReactor_wait(yoReactor *reactor, struct timeval *timeo);

//Factory函数声明
int yoFactory_create(yoFactory *factory);
int yoFactory_start(yoFactory *factory);
int yoFactory_shutdown(yoFactory *factory);
int yoFactory_dispatch(yoFactory *factory, yoEventData *req);
int yoFactory_finish(yoFactory *factory, yoSendData *resp);
int yoFactory_check_callback(yoFactory *factory);

//FactoryProcess函数声明
int yoFactoryProcess_start(yoFactory *factory);
int yoFactoryProcess_shutdown(yoFactory *factory);
int yoFactoryProcess_finish(yoFactory *factory, yoSendData *resp);
int yoFactoryProcess_dispatch(yoFactory *factory, yoEventData *data);
int yoFactoryProcess_create(yoFactory *factory, int writer_num, int worker_num);



int yoRead(int fd, char *buf, int count);
int yoWrite(int fd, char *buf, int count);
void yoSetNonBlock(int sock);
void yoSetBlock(int sock);








#endif
