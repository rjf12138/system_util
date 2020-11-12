#ifndef __MUTEX_H__
#define __MUTEX_H__

#include "basic_head.h"

namespace system_util {

class Mutex : public MsgRecord {
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

}
#endif