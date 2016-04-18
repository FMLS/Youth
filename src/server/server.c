#include <signal.h>
#include "youth.h"
#include "server.h"

//static int child_fork = 0;
static int yoServer_check_callback(yoServer *serv);
static int yoServer_poll_start(yoServer *serv);
static int yoServer_poll_loop(yoThreadParam *param);
static int yoServer_poll_onClose(yoReactor *reactor, yoEvent *event);
static int yoServer_poll_onReceive(yoReactor *reactor, yoEvent *event);

int yoServer_close(yoServer *serv, yoEvent *event);

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
    serv->event_fd = eventfd(0, EFD_NONBLOCK);
    if (serv->event_fd < 0) {
       yoTrace("create event_fd failed \n");
       return --step;
    }
    serv->threads = yo_calloc(serv->poll_thread_num, sizeof(yoThreadPoll));
    if (serv->threads == NULL) {
        yoTrace("ThreadPoll calloc failed\n");
        return --step;
    }
    if (serv->factory_mode == YO_MODE_PROCESS) {
        if (serv->writer_num < 1 || serv->worker_num < 1) {
            yoTrace("serv->writer_num < 1 or serv->worker_num < 1 \n");
            return --step;
        }
        ret = yoFactoryProcess_create(&(serv->factory), serv->writer_num, serv->worker_num);
    }
    else {
        ret = yoFactory_create(&(serv->factory));
    }
    if (ret < 0) {
        yoTrace("create factory fail\n");
        return --step;
    }
    serv->factory.ptr = serv;
    serv->factory.onTask = serv->onReceive;
//    serv->factory.onFinish = yoServer_onFinish;
    return YO_OK;

}

int yoServer_start(yoServer *serv)
{
    int option;
    int ret = 0, step = 0;
    //未测试
    yoReactor *main_reactor = serv->main_reactor;
    yoFactory *factory = &(serv->factory);
    struct timeval tmo;
    struct sockaddr_in server_addr;

    ret = yoServer_check_callback(serv);
    if (ret < 0) {
        yoTrace("check_callback failed!\n");
        return --step;
    }

    if (serv->daemonize > 0) {
        if (daemon(1, 1) < 0) { //为了Trace输出
            return YO_ERR;
        }
    }

    ret = factory->start(factory);
    if (ret < 0) {
        yoTrace("factory start failed! \n");
        return --step;
    }

    ret = yoServer_poll_start(serv);
    if (ret < 0) {
        yoTrace("poll start failed! \n");
        return --step;
    }
    bzero(&server_addr, sizeof(struct sockaddr_in));
    inet_aton(serv->host, &(server_addr.sin_addr));
    server_addr.sin_port = htons(serv->port);
    server_addr.sin_family = AF_INET;
    yoTrace("Bind host=%s, port=%d\n", serv->host, serv->port);
    serv->sock = socket(PF_INET, SOCK_STREAM, 0);
    option = 1;
    setsockopt(serv->sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int));
    if (serv->sock < 0) {
        yoTrace("[yoServerCreate] create socket fail! \n");
        return --step;
    }
    
    ret = bind(serv->sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
    if (ret !=  0) {
        yoTrace("bind failed \n");
        return --step;
    }

    ret = listen(serv->sock, serv->backlog);
    if (ret != 0) {
        yoTrace("listen fail \n");
        return --step;
    }

    ret = yoSelectReactor_create(main_reactor);
    if (ret < 0) {
        yoTrace("SelectReactor_create fail \n");
        return --step;
    }

    main_reactor->ptr = serv;
//    serv->main_reactor = &main_reactor;

    tmo.tv_sec = 5;
    tmo.tv_usec = 0;

    main_reactor->setHandle(main_reactor, YO_EVENT_CLOSE, yoServer_onClose);
    main_reactor->setHandle(main_reactor, YO_EVENT_CONNECT, yoServer_onAccept);
    main_reactor->add(main_reactor, serv->event_fd, YO_EVENT_CLOSE);
    main_reactor->add(main_reactor, serv->sock, YO_EVENT_CONNECT);

    serv->onStart(serv);
    main_reactor->wait(main_reactor, &tmo);
    serv->onShutdown(serv);
    return YO_OK;
}

int yoServer_onClose(yoReactor *reactor, yoEvent *event)
{
    int ret;
    yoServer *serv = reactor->ptr;
    yoEventClose cev;
    yoReactor *from_reactor;
    ret = read(serv->event_fd, &cev, sizeof(uint64_t));
    if (ret < 0) {
        return YO_ERR;
    }
    serv->onClose(serv, cev.fd, cev.from_id);
    from_reactor = &(serv->threads[cev.from_id].reactor);
    from_reactor->del(from_reactor, cev.fd);
    ret = close(cev.fd);
    return ret;
}



int yoServer_onAccept(yoReactor *reactor, yoEvent *event)
{
    int clilen;
    int conn_fd;
    int ret = 0;
    char *str = NULL;
    yoServer *serv = reactor->ptr;
    struct sockaddr_in client_addr;
    clilen = sizeof(client_addr);
    yoTrace("[mainReactor] accept start \n");
    conn_fd = accept(serv->sock, (struct sockaddr *)&client_addr, (socklen_t*)&clilen);
    if (conn_fd < 0) {
        yoTrace("[mainReactor] accept failed Errno: %d | sock_fd: %d \n", errno, serv->sock);
        return YO_ERR;
    }
    yoSetNonBlock(conn_fd);
    str = inet_ntoa(client_addr.sin_addr);
    if (serv->c_pti >= serv->poll_thread_num) {
        serv->c_pti = 0;
    }
    ret = serv->threads[serv->c_pti].reactor.add(&(serv->threads[serv->c_pti].reactor), conn_fd, YO_FD_CONN);
    if (ret < 0) {
        yoTrace("[mainReactor] add event to thread's reactor failed! Errno: %d | fd: %d \n",errno, conn_fd);
        return YO_ERR;
    }
    printf("accept: %d\n", conn_fd);
    serv->onConnect(serv, conn_fd, serv->c_pti);
    serv->c_pti++;

    return YO_OK;
}

static int yoServer_check_callback(yoServer *serv)
{
    int step = 0;
    if (serv->onStart == NULL) {
        return --step;
    }
    if (serv->onConnect == NULL) {
        return -- step;
    }
    if (serv->onReceive == NULL) {
        return --step;
    }
    if (serv->onClose == NULL) {
        return --step;
    }
    if (serv->onShutdown == NULL) {
        return -- step;
    }
    return YO_OK;
}

static int yoServer_poll_start(yoServer *serv)
{
    int i = 0;
    pthread_t pidt;
    yoThreadParam *param;

    for (i = 0; i < serv->poll_thread_num; i++) {
        param = yo_malloc(sizeof(yoThreadParam));
        if (param == NULL) {
            yoTrace("malloc yoThreadParam failed\n");
            return YO_ERR;
        }
        param->object = serv;
        param->pti = i;
        pthread_create(&pidt, NULL, (void* (*)(void*)) yoServer_poll_loop, (void*)param);
        serv->threads[i].ptid = pidt;
    }
    return YO_OK;
}

static int yoServer_poll_loop(yoThreadParam *param)
{
    struct timeval tmo;
    int ret = 0, pti = param->pti;
    yoServer *serv = param->object;
    yoReactor *reactor = &(serv->threads[pti].reactor);
    reactor->ptr = serv;
    ret = yoSelectReactor_create(reactor);
    if (ret < 0) {
        yoTrace("SelectReactor_create failed\n");
        return YO_ERR;
    }
    tmo.tv_sec = serv->timeout_sec;
    tmo.tv_usec = serv->timeout_usec;
//    reactor->setHandle(reactor, YO_FD_CLOSE, yoServer_poll_onClose);
    reactor->setHandle(reactor, YO_FD_CONN, yoServer_poll_onReceive);
    reactor->wait(reactor, &tmo);
    reactor->free(reactor);
    yo_free(param);
    return YO_OK;
}

static int yoServer_poll_onClose(yoReactor *reactor, yoEvent *event)
{
    yoServer *serv = reactor->ptr;
    return yoServer_close(serv, event);
}

static int yoServer_poll_onReceive(yoReactor *reactor, yoEvent *event)
{
    int ret, n;
    yoEventData from_client;
    yoServer *serv = reactor->ptr;
    yoFactory *factory = &(serv->factory);
    bzero(from_client.data, sizeof(from_client));
    ret = yoRead(event->fd, from_client.data, YO_BUFFER_SIZE);
    if (ret < 0) {
            yoTrace("read fd error\n");
        return YO_ERR;
    }
    else if (ret == 0) {
        yoTrace("Close event.fd:%d | from: %d\n", event->fd, event->from_id);
        reactor->del(reactor, event->fd);
        close(event->fd);
//        return yoServer_close(serv, event);
    }
    else {
       from_client.fd = event->fd;
       from_client.len = ret;
       from_client.from_id = event->from_id;
       yoTrace("before dispatch\n");
       n = factory->dispatch(factory, &from_client);
    }
    return YO_OK;
}

int yoServer_close(yoServer *serv, yoEvent *event)
{
    yoEventClose cev;
    cev.fd = event->fd;
    cev.from_id = event->from_id;
    return write(serv->event_fd, &cev, sizeof(cev));
}

int yoServer_onFinish(yoFactory *factory, yoSendData *resp)
{
    return yoWrite(resp->fd, resp->data, resp->len);
}
















