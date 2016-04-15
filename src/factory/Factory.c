#include "youth.h"

int yoFactory_create(yoFactory *factory)
{
    factory->dispatch = yoFactory_dispatch;
    factory->finish   = yoFactory_finish;
    factory->start    = yoFactory_start;
    factory->shutdown = yoFactory_shutdown;
    return YO_OK;
}

int yoFactory_start(yoFactory *factory)
{
    return YO_OK;
}

int yoFactory_shutdown(yoFactory *factory)
{
    return YO_OK;
}

int yoFactory_dispatch(yoFactory *factory, yoEventData *req)
{
    int ret;
    yoTrace("New Task: %s\n", req->data);
    ret = factory->onTask(factory, req);
    return ret;
}

int yoFactory_finish(yoFactory *factory, yoSendData *resp)
{
    return factory->onFinish(factory, resp);
}

int yoFactory_check_callback(yoFactory *factory)
{
    int step = 0;
    if (factory->onTask == NULL) {
        return --step;
    }
    if (factory->onFinish == NULL) {
        return --step;
    }
    return YO_OK;
}
