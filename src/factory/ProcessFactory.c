#include <signal.h>
#include "youth.h"


static int c_worker_pipe = 0; // this process_worker's pipe

int yoFactoryProcess_start(yoFactory *factory);
static int yoFactoryProcess_writer_start(yoFactory *factory);
int yoFactoryProcess_writer_receive(yoReactor *reactor, yoEvent *event);
int yoFactoryProcess_writer_loop(yoThreadParam *param);
static int yoFactoryProcess_worker_start(yoFactory *factory);
static int yoFactoryProcess_worker_spawn(yoFactory *factory, int writer_id, int worker_id);
static int yoFactoryProcess_worker_loop(yoFactory *factory, int c_pipe);
int yoFactoryProcess_shutdown(yoFactory *factory);
int yoFactoryProcess_finish(yoFactory *factory, yoSendData *resp);
int yoFactoryProcess_dispatch(yoFactory *factory, yoEventData *data);
int yoFactoryProcess_create(yoFactory *factory, int writer_num, int worker_num);
    

int yoFactoryProcess_create(yoFactory *factory, int writer_num, int worker_num)
{
    int step = 0;
    yoFactoryProcess *this = yo_malloc(sizeof(yoFactoryProcess));
    if (this == NULL) {
        yoTrace("[yoFactoryProcess_create] malloc[0] failed \n");
        return --step;
    }
    this->writers = yo_calloc(writer_num, sizeof(yoThreadWriter));
    if (this->writers == NULL) {
        yoTrace("[yoFactoryProcess_create] malloc[1] failed \n");
        return --step;
    }
    this->workers = yo_calloc(worker_num, sizeof(yoWorker));
    if (this->workers == NULL) {
        yoTrace("[yoFactoryProcess_create] malloc[0] failed \n");
        return --step;
    }
    
    this->writer_num = writer_num;
    this->worker_num = worker_num;
    this->c_writer_id = 0;
    this->c_worker_id = 0;
    factory->object = this;
    factory->dispatch = yoFactoryProcess_dispatch;
    factory->finish   = yoFactoryProcess_finish;
    factory->start    = yoFactoryProcess_start;
    factory->shutdown = yoFactoryProcess_shutdown;
    factory->onTask   = NULL;
    factory->onFinish = NULL;
    return YO_OK;
}

int yoFactoryProcess_start(yoFactory *factory)
{
    int ret, step = 0;
//    ret = yoFactory_check_callback(factory);
//    if (ret < 0) {
//        return --step;
//    }
    ret = yoFactoryProcess_writer_start(factory);
    if (ret < 0) {
        return --step;
    }
//    ret = yoFactoryProcess_worker_start(factory);
//    if (ret < 0) {
//        return --step;
//    }
    return YO_OK;
}

static int yoFactoryProcess_writer_start(yoFactory *factory)
{
    int i = 0, ret = 0;
    pthread_t thread_id;
    yoThreadParam *param;
    yoFactoryProcess *this = factory->object;

    for (i = 0; i < this->writer_num; i++) {
        param = yo_malloc(sizeof(yoThreadParam));
        if (param == NULL) {
            yoTrace("[yoFactoryProcess_writer_start] malloc failed\n");
            return YO_ERR;
        }
        param->object = factory;
        param->pti = i;
        ret = pthread_create(&thread_id, NULL, (void* (*)(void *))yoFactoryProcess_writer_loop, (void *)param);
        if (ret < 0) {
            yoTrace("[yoFactoryProcess_writer_start] pthread_create failed \n");
            return YO_ERR;
        }
        this->writers[i].ptid = thread_id;
        YO_START_SLEEP;
    }

    //待解决 感觉有问题
//    for (i = 0; i < this->writer_num; i++) {
//        pthread_join(this->writers[i].ptid, NULL);
//    }

    return YO_OK;
}

int yoFactoryProcess_writer_receive(yoReactor *reactor, yoEvent *event)
{
    char buf[1000];
    int ret = 0;
    ret = read(event->fd, buf, 100);
    if (ret < 0) {
        strcpy(buf, "read failed\n");
    }
    printf("fd: %d | from_id: %d | type: %d |msg : %s \n", event->fd, event->from_id, event->type, buf);
    return YO_OK;
}

int yoFactoryProcess_writer_loop(yoThreadParam *param)
{
    int pti = param->pti;
    yoFactory *factory = param->object;
    yoFactoryProcess *this = factory->object;
    yoReactor *reactor = &(this->writers[pti].reactor);
    struct timeval tmo;
    tmo.tv_sec = 3;
    tmo.tv_usec = 0;
    reactor->factory = factory;
    reactor->id = pti;

    if (yoSelectReactor_create(reactor) < 0) {
        yoTrace("[yoFactoryProcess_writer_loop] yoReacotrSelect_create failed \n");
        pthread_exit((void*)param);
    }

    reactor->setHandle(reactor, YO_FD_CONN, yoFactoryProcess_writer_receive);
    reactor->wait(reactor, &tmo);
    reactor->free(reactor);
    pthread_exit((void*)param);
    return YO_OK;
}

static int yoFactoryProcess_worker_start(yoFactory *factory)
{
    int i = 0 , ret = 0;
    yoFactoryProcess *this = factory->object;
    for (i = 0; i < this->writer_num; i++) {
        yoTrace("[yoFactoryProcess_worker_start] \n");
        ret = yoFactoryProcess_worker_spawn(factory, (i % this->writer_num), i);
    }
    return YO_OK;
}

static int yoFactoryProcess_worker_spawn(yoFactory *factory, int writer_id, int worker_id)
{
    pid_t pid;
    int pipe[2];
    yoFactoryProcess *this = factory->object;
    return YO_OK;
}

static int yoFactoryProcess_worker_loop(yoFactory *factory, int c_pipe)
{
    
    return YO_OK;
}


int yoFactoryProcess_shutdown(yoFactory *factory)
{

    return YO_OK;
}

int yoFactoryProcess_finish(yoFactory *factory, yoSendData *resp)
{

    return YO_OK;
}

int yoFactoryProcess_dispatch(yoFactory *factory, yoEventData *data)
{

    return YO_OK;
}


