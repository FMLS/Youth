#include "youth.h"
#include "server.h"
int tmp_pipe = 0;
int myHeadle(yoReactor *reactor, yoEvent *event) {
    char buf[100];
    read(tmp_pipe, buf, 100); 
    printf("fd: %d | from_id: %d | type: %d \n | msg: %s", event->fd, event->from_id, event->type, buf); 
    return 0;
}

int myTask(yoFactory *factory, yoEventData *event)
{
    int ret = 0;
    char buf[10];
    yoSendData *send_data = (yoSendData*)malloc(sizeof(yoSendData));
    sprintf(buf, "%d", event->fd);
    strcpy(send_data->data, "hello i am Task, I get msg from client:  \n");
    strcat(send_data->data, buf);
    printf("%s \n", send_data->data);
    send_data->len = strlen(send_data->data)+1;

    factory->finish(factory, send_data);
    return 0;
}


int my_onReceive(yoFactory *factory, yoEventData *req);
void my_onStart(yoServer *serv);
void my_onShutdown(yoServer *serv);
void my_onConnect(yoServer *serv, int fd,int from_id);
void my_onClose(yoServer *serv, int fd,int from_id);

int main()
{
    youth_running=1;
    //    int ret = 0;
    //
    //    int mpipe1[2], mpipe2[2], mpipe3[2], mpipe4[2];
    //    pid_t pid;
    //    yoFactory myFactory;  
    //    yoFactoryProcess_create(&myFactory, 3, 4);
    //    myFactory.onTask = myTask;
    //    yoFactoryProcess_start(&myFactory);
    //    //    yoFactoryProcess *factory = myFactory.object;
    //    yoEventData data;
    //    char bufa[100] = {0}, bufb[100] = {0}, bufc[100]={0}, bufd[100] = {0};
    //
    //
    //    pipe(mpipe1);
    //    pipe(mpipe2);
    //    pipe(mpipe3);
    //    pipe(mpipe4);
    //    pid = fork();
    //
    //    if (pid == 0) {
    //        close(mpipe1[0]);
    //        close(mpipe2[0]);
    //        close(mpipe3[0]);
    //        close(mpipe4[0]);
    //        char *buf1 = "I am client-1, I put this message-1.";
    //        char *buf2 = "I am client-2, I put this message-2.";
    //        char *buf3 = "I am client-3, I put this message-3.";
    //        char *buf4 = "I am client-4, I put this message-4.";
    //
    //
    //        while (1) {
    //            sleep(3);
    //            write(mpipe1[1], buf1, strlen(buf1) + 1);
    //            sleep(1);
    //            write(mpipe2[1], buf2, strlen(buf2) + 1);
    //            sleep(1);
    //            write(mpipe3[1], buf3, strlen(buf3) + 1);
    //            sleep(1);
    //            write(mpipe4[1], buf4, strlen(buf4) + 1);
    //            sleep(1);
    //        }
    //    }
    //    else {
    //     printf("%d-%d \n", mpipe1[0], mpipe1[1]);
    //    printf("%d-%d \n", mpipe2[0], mpipe2[1]);
    //    printf("%d-%d \n", mpipe3[0], mpipe3[1]);
    //    printf("%d-%d \n", mpipe4[0], mpipe4[1]);
    //
    //        close(mpipe1[1]);
    //        close(mpipe2[1]);
    //        close(mpipe3[1]);
    //        close(mpipe4[1]);
    //
    //        while(1) {
    //            sleep(3);
    //             data.fd = 1;
    //             data.from_id = 1;
    //             ret = read(mpipe1[0], bufa, 100);
    //             strcpy(data.data, bufa);
    //             myFactory.dispatch(&myFactory, &data);
    //             sleep(1);
    //             data.fd = 2;
    //             data.from_id = 2;
    //             read(mpipe2[0], bufb, 100);
    //             strcpy(data.data, bufb);
    //             myFactory.dispatch(&myFactory, &data);
    //             sleep(1);
    //             data.fd = 3;
    //             data.from_id = 3;
    //             read(mpipe3[0], bufc, 100);
    //             strcpy(data.data, bufc);
    //             myFactory.dispatch(&myFactory, &data);
    //             sleep(1);
    //
    //             data.fd = 4;
    //             data.from_id = 4;
    //             read(mpipe4[0], bufd, 100);
    //             strcpy(data.data, bufd);
    //             myFactory.dispatch(&myFactory, &data);
    //             sleep(1);
    //        }
    //       while(1) { 
    //        sleep(3);
    //
    //        read(mpipe1[0], bufa, 100);
    //        read(mpipe2[0], bufb, 100);
    //        read(mpipe3[0], bufc, 100);
    //       }

    //        tmp_pipe = mpipe[0];
    //        for (i = 0; i < factory->writer_num; i++) {
    //            factory->writers[i].reactor.add(&(factory->writers[i].reactor), mpipe[0], YO_FD_CONN);
    //        }
    //        wait(NULL);
    //        while(1);
    //        return 0;
    //    }
    int ret = 0;
    yoServer serv;
    yoServer_init(&serv);
    serv.port = 9500;
    serv.host = "127.0.0.1";
    serv.backlog = 128;
    serv.poll_thread_num = 4;
    serv.writer_num = 4;
    serv.worker_num = 4;
    serv.factory_mode = 3;

    serv.onStart = my_onStart;
    serv.onShutdown = my_onShutdown;
    serv.onConnect = my_onConnect;
    serv.onReceive = my_onReceive;
    serv.onClose = my_onClose;

    ret = yoServer_create(&serv);
    if (ret < 0)
    {
        yoTrace("create server fail[error=%d].\n", ret);
        exit(0);
    }
    ret = yoServer_start(&serv);
    if (ret < 0)
    {
        yoTrace("start server fail[error=%d].\n", ret);
        exit(0);
    }
    return 0;

}

int my_onReceive(yoFactory *factory, yoEventData *req)
{
    printf("In my_onReceive\n");
    int ret = 0;
    char resp_data[YO_BUFFER_SIZE];
    yoSendData resp;
    resp.fd = req->fd; //fd can be not source fd.
    resp.len = req->len + 8;

    yoTrace("Data Len=%d\n", req->len);
    snprintf(resp_data, resp.len, "%s", req->data);
    resp.data = resp_data;
    ret = factory->finish(factory, &resp);
    yoTrace("finish\n");
    return YO_OK;
}

void my_onStart(yoServer *serv)
{
    printf("Server is running\n");
}

void my_onShutdown(yoServer *serv)
{
    printf("Server is shutdown\n");
}

void my_onConnect(yoServer *serv, int fd,int from_id)
{
    printf("Connect fd=%d|from_id=%d\n", fd, from_id);
}

void my_onClose(yoServer *serv, int fd,int from_id)
{
    printf("Close fd=%d|from_id=%d\n", fd, from_id);
}

