#include "mutex.h"

Mutex::Mutex(void) 
{
    ::pthread_mutex_init(&mutex_, NULL);
}

Mutex::~Mutex(void)
{
    ::pthread_mutex_destroy(&mutex_);
}

int 
Mutex::lock(void)
{
    return  ::pthread_mutex_lock(&mutex_);
}

int 
Mutex::trylock(void)
{
    return ::pthread_mutex_trylock(&mutex_);
}

int 
Mutex::unlock(void)
{
    return ::pthread_mutex_unlock(&mutex_);
}