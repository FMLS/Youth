#ifndef __YO_SERVER_H
#define __YO_SERVER_H

#include "youth.h"

#define YO_EVENT_CLOSE   5
#define YO_EVENT_CONNECT 6

typedef struct __yoServer yoServer;

typedef struct __yoTreadPoll
{
    pthread_t ptid;
    yoReactor reactor;
}yoThreadPoll;


struct __yoServer
{
    char *host;
    int port;
    int backlog;
    int max_conn;
    int daemonize;
    int writer_num;
    int worker_num;
    int factory_mode;
    int timeout_sec;
    int timeout_usec;
    int poll_thread_num;

    int sock;
    int event_fd;
    int timer_fd;
    int signal_fd;

    int c_pti;

    yoReactor    reactor; //可能没用
    yoFactory    factory;
    yoThreadPoll *threads;

    void *ptr;

    void (*onStart)     (yoServer *serv);
    void (*onConnect)   (yoServer *serv, int fd, int from_id);
    int  (*onReceive)   (yoServer *serv, yoEventData *data);
    void (*onClose)     (yoServer *serv, int fd, int from_id);
    void (*onShutdown)  (yoServer *serv);

};























#endif
