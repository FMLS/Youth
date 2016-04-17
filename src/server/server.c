#include <signal.h>
#include "youth.h"
#include "server.h"

static int child_fork = 0;

void yoServer_init(yoServer *serv)
{
    bzero(serv, sizeof(yoServer));
    serv->backlog = YO_BACKLOG;
    serv->max_conn = YO_MAX_FDS;
    serv->factory_mode = YO_MODE_PROCESS;
    serv->poll_thread_num = YO_THREAD_NUM;

    serv->daemonize = 0;

    serv->timeout_sec = 1;
    serv->timeout_usec = 0;

    serv->writer_num = YO_CPU_NUM;
    serv->worker_num = YO_CPU_NUM;

    
    serv->onStart    = NULL;
    serv->onConnect  = NULL;
    serv->onReceive  = NULL;
    serv->onShutdown = NULL;
    serv->onClose    = NULL;

}

int yoServer_create(yoServer *serv)
{
    int ret = 0, step = 0;
    serv->event_fd = eventfd(

}

int yoServer_start(yoServer *serv)
{


}


