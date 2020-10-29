#ifndef __MUTEX_H__
#define __MUTEX_H__

#include "basic_head.h"

class Mutex {
public:
    Mutex(void);
    virtual ~Mutex(void);

    virtual int lock(void);
    virtual int trylock(void);
    virtual int unlock(void);

    virtual int get_errno(void) { return 0;};
private:
    int errno_;
    pthread_mutex_t mutex_;
};

#endif