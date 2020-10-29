#ifndef __THREAD_H__
#define __THREAD_H__

#include "basic_head.h"
#include "mutex.h"

class Thread {
public:
    Thread(void);
    virtual ~Thread(void);

    static void* create_func(void *arg);

    virtual int init(void);
    virtual int run_handler(void);
    virtual int stop_handler(void);
    virtual int start_handler(void);
    virtual int wait_thread(void);

private:
    pthread_t thread_id_;
};

#endif