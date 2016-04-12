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
    int mpipe[2];
    pid_t pid;
    struct timeval tmo;
    yoReactor test;
    yoReactor *reactor = &test;
    yoSelectReactor_create(reactor);
    reactor->setHandle(reactor, 1, myHeadle);

    pipe(mpipe);
    pid = fork();

    if (pid == 0) {
        while (1) {
            char *buf = "hello I am child \n";
            close(mpipe[0]);
            write(mpipe[1], buf, strlen(buf) + 1);
            sleep(1);
        }
    }
    else {
        close(mpipe[1]);
        tmp_pipe = mpipe[0];
        tmo.tv_sec = 3;
        tmo.tv_usec = 0;
        reactor->add(reactor, mpipe[0], 1);
        reactor->wait(reactor, &tmo);
    }
    return 0;
}
