#include "system_utils.h"

namespace system_utils {

Thread::Thread(void) 
{
    this->set_stream_func(LOG_LEVEL_TRACE, g_msg_to_stream_trace);
    this->set_stream_func(LOG_LEVEL_DEBUG, g_msg_to_stream_debug);
    this->set_stream_func(LOG_LEVEL_INFO, g_msg_to_stream_info);
    this->set_stream_func(LOG_LEVEL_WARN, g_msg_to_stream_warn);
    this->set_stream_func(LOG_LEVEL_ERROR, g_msg_to_stream_error);
    this->set_stream_func(LOG_LEVEL_FATAL, g_msg_to_stream_fatal);
}

Thread::~Thread(void) { }

void* 
Thread::create_func(void* arg)
{
    if (arg == NULL) {
        return NULL;
    }

    Thread *self = (Thread*)arg;
    self->run_handler();

    return arg;
}

int 
Thread::init(void)
{
    this->start_handler();
#ifdef __RJF_LINUX__
    int ret = ::pthread_create(&thread_id_, NULL, create_func, (void*)this);
    if (ret != 0) {
        LOG_ERROR("pthread_create(): %s", strerror(ret));
        return ret;
    }
#endif

    return 0;
}

int Thread::run_handler(void) { return 0;}
int Thread::stop_handler(void) {return 0; }
int Thread::start_handler(void) {return 0; }

int 
Thread::wait_thread(void)
{
#ifdef __RJF_LINUX__
    int ret = ::pthread_join(thread_id_, NULL);
    if (ret != 0) {
        LOG_ERROR("pthread_join(): %s", strerror(ret));
        return ret;
    }
#endif

    return 0;
}


WorkThread::WorkThread(ThreadPool *thread_pool)
: exit_(false), max_life_(30), thread_pool_(thread_pool)
{
    thread_id_ = (int64_t)this;
#ifdef __RJF_LINUX__
    pthread_cond_init(&thread_cond_, NULL);
#endif
}

WorkThread::~WorkThread(void)
{
#ifdef __RJF_LINUX__
    pthread_cond_destroy(&thread_cond_);
#endif
}

void 
WorkThread::thread_cond_wait(void)
{
#ifdef __RJF_LINUX__
    pthread_cond_wait(&thread_cond_, mutex_.get_mutex());
#endif
}

void 
WorkThread::thread_cond_signal(void)
{
#ifdef __RJF_LINUX__
    pthread_cond_signal(&thread_cond_);
#endif
}

int 
WorkThread::run_handler(void)
{
    Task task;
    while (!exit_) {
        mutex_.lock();
        while (state_ == WorkThread_WAITING) {
            thread_cond_wait();
        }
        mutex_.unlock();

        if (thread_pool_->get_task(task) > 0) {
            task.state = THREAD_TASK_HANDLE;
            task.work_func(task.arg);
            task.state = THREAD_TASK_COMPLETE;
        }

        this->pause();
    }
}

int 
WorkThread::stop_handler(void)
{
    exit_ = true;
}

int 
WorkThread::start_handler(void)
{
    exit_ = false;
}

int 
WorkThread::pause(void)
{
    mutex_.lock();
    state_ = WorkThread_WAITING;
    mutex_.unlock();
}

int 
WorkThread::resume(void)
{
    mutex_.lock();
    state_ = WorkThread_RUNNING;
    thread_cond_signal();
    mutex_.unlock();
}

int 
WorkThread::set_max_life(int life)
{

}


}
///////////////////////////////////////////////////////////////////////////////////