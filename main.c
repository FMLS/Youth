#include "youth.h"

int tmp_pipe = 0;
int myHeadle(yoReactor *reactor, yoEvent *event) {
   char buf[100];
   read(tmp_pipe, buf, 100); 
   printf("fd: %d | from_id: %d | type: %d \n | msg: %s", event->fd, event->from_id, event->type, buf); 
    return 0;
}


int main()
{
      youth_running=1;
//TEST reactor

//    int mpipe[2];
//    pid_t pid;
//    struct timeval tmo;
//    yoReactor test;
//    yoReactor *reactor = &test;
//    yoSelectReactor_create(reactor);
//    reactor->setHandle(reactor, 1, myHeadle);
//
//    pipe(mpipe);
//    pid = fork();
//
//    if (pid == 0) {
//        while (1) {
//            char *buf = "hello I am child \n";
//            close(mpipe[0]);
//            write(mpipe[1], buf, strlen(buf) + 1);
//        }
//    }
//    else {
//        close(mpipe[1]);
//        tmp_pipe = mpipe[0];
//        tmo.tv_sec = 3;
//        tmo.tv_usec = 0;
//        reactor->add(reactor, mpipe[0], 1);
//        reactor->wait(reactor, &tmo);
//    }
//END TEST

    int mpipe[2];
    int i = 0;
    pid_t pid;
    yoFactory myFactory;  
    yoFactoryProcess_create(&myFactory, 10, 1);
    yoFactoryProcess_start(&myFactory);
    yoFactoryProcess *factory = myFactory.object;


    pipe(mpipe);
    pid = fork();
 
    if (pid == 0) {
        while (1) {
            sleep(1);
            char *buf = "hello i am msg , I go to here to test this process, if every thread can get me totally, that is very amazing!\n";
            close(mpipe[0]);
            write(mpipe[1], buf, strlen(buf) + 1);
        }
    }
    else {
        close(mpipe[1]);
        tmp_pipe = mpipe[0];
        for (i = 0; i < factory->writer_num; i++) {
            factory->writers[i].reactor.add(&(factory->writers[i].reactor), mpipe[0], YO_FD_CONN);
        }
    }
    wait(NULL);
    return 0;
}
