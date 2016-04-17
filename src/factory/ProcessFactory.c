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
    ret = yoFactoryProcess_worker_start(factory);
    if (ret < 0) {
        return --step;
    }
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
    yoSendData resp;
    printf("event_fd %d \n", event->fd);
    ret = read(event->fd, &resp, sizeof(resp));
    printf("sizeof resp: %d", sizeof(resp));
    if (ret < 0) {
        strcpy(buf, "read failed\n");
        return -1;
    }
    strcpy(buf, resp.data);
    printf("Reactor_id: %d msg: %s", reactor->id, resp.data);
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
    for (i = 0; i < this->worker_num; i++) {
        yoTrace("[yoFactoryProcess_worker_start] \n");
        ret = yoFactoryProcess_worker_spawn(factory, (i % this->writer_num), i);
    }
    return YO_OK;
}

static int yoFactoryProcess_worker_spawn(yoFactory *factory, int writer_id, int worker_id)
{
    pid_t pid;
    int ret = 0;
    int pipe[2];
    yoFactoryProcess *this = factory->object;
    ret = socketpair(PF_LOCAL, SOCK_DGRAM, 0, pipe);
    if (ret < 0) {
        yoTrace("[yoFactoryProcess_worker_spaw] create unix socket failed \n");
        return YO_ERR;
    }

    pid = fork();
    if (pid < 0) {
        yoTrace("[yoFactoryProcess_worker_spaw] fork worker process failed \n");
        exit(5);
    }
    else if (pid == 0){
        close(pipe[0]);
        yoFactoryProcess_worker_loop(factory, pipe[1]);
        exit(0);
    }
    else {
        close(pipe[1]);
        this->writers[writer_id].reactor.add(&(this->writers[writer_id].reactor), pipe[0], YO_FD_CONN);
        this->workers[worker_id].pipe_fd = pipe[0];
        return pid;
    }
}

static int yoFactoryProcess_worker_loop(yoFactory *factory, int c_pipe)
{
    int n = 0;
    yoEventData req;
    c_worker_pipe = c_pipe;
    while (1) {
        n = read(c_pipe, &req, sizeof(req));
        printf("[Worker: %d]Recv: pipe: %d | pti: %d\n",getpid(), c_pipe, req.from_id);
        if (n > 0) {
            factory->onTask(factory, &req);
        }
        else {
            yoTrace("[Worker] read pipe error \n");
        }
    }
    
    return YO_OK;
}


int yoFactoryProcess_shutdown(yoFactory *factory)
{
    int i = 0;
    pid_t worker_pid;
    yoFactoryProcess *this = factory->object;
    for (i = 0; i < this->worker_num; i++) {
        worker_pid = this->workers[i].pid;
        kill(worker_pid, SIGTERM);
        yoTrace("[shutdown] kill worker process [pid: %d ]\n", worker_pid);
    }
    free(this->workers);
    free(this->writers);
    free(this);

    return YO_OK;
}

int yoFactoryProcess_finish(yoFactory *factory, yoSendData *resp)
{
    yoEventData send_data;
    memcpy(send_data.data, resp->data, resp->len);
    send_data.fd = resp->fd;
    send_data.len = resp->len;
    write(c_worker_pipe, &send_data, resp->len + (3 * sizeof(int)));
    free(resp);
    return YO_OK;
}

int yoFactoryProcess_dispatch(yoFactory *factory, yoEventData *data)
{
    int ret = 0;
    yoFactoryProcess *this = factory->object;

    if (this->c_worker_id >= this->worker_num) {
        this->c_worker_id = 0;
    }
    yoTrace("[ReadThread] send to: pipe= %d | worker= %d\n", this->workers[this->c_worker_id].pipe_fd, this->c_worker_id);
    ret = write(this->workers[this->c_worker_id].pipe_fd, data, data->len + (3 * sizeof(int)));
    printf("dispatch id: %d\n", data->from_id);
    if (ret < 0) {
        printf("dispatch error !\n");
        return YO_ERR;
    }
    this->c_worker_id++;

    return YO_OK;
}


