#include "youth.h"

int yoReactor_accept(yoReactor *reactor, yoEvent * event)
{

    return YO_OK;
}


int yoReactor_close(yoReactor *reactor, yoEvent *event)
{

    return YO_OK;
}

int yoReactor_setHandle(yoReactor *reactor, int fdtype, yoReactor_handle handle)
{
    if (fdtype >= YO_MAX_FDTYPE) {
        return -1;
    }
    else {
        reactor->handle[fdtype] = handle;
        return 0;
    }

}

int yoReactor_receive(yoReactor *reactor, yoEvent *event)
{

    return YO_OK;
}
