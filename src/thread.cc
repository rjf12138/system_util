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

////////////////////////////////////// WorkThread ///////////////////////////////////////
WorkThread::WorkThread(ThreadPool *thread_pool, int idle_life)
: idle_life_(idle_life), thread_pool_(thread_pool)
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
    while (state_ != WorkThread_EXIT) {
        mutex_.lock();
        while (state_ == WorkThread_WAITING) {
            thread_cond_wait();
        }
        mutex_.unlock();

        if (state_ == WorkThread_RUNNING) {
            if (thread_pool_->get_task(task) > 0) {
                task.state = THREAD_TASK_HANDLE;
                task.work_func(task.arg);
                task.state = THREAD_TASK_COMPLETE;
            }
        }

        this->pause();
    }

    return 0;
}

int 
WorkThread::stop_handler(void)
{
    state_ = WorkThread_EXIT;

    return 0;
}

int 
WorkThread::start_handler(void)
{
    state_ = WorkThread_RUNNING;

    return 0;
}

int 
WorkThread::pause(void)
{
    if (state_ == WorkThread_RUNNING) {
        mutex_.lock();
        state_ = WorkThread_WAITING;
        remain_life_ = idle_life_;
        mutex_.unlock();
    }

    return 0;
}

int 
WorkThread::resume(void)
{
    if (state_ == WorkThread_WAITING) {
        mutex_.lock();
        state_ = WorkThread_RUNNING;
        thread_cond_signal();
        mutex_.unlock();
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////
ThreadPool::ThreadPool(void)
: exit_(false)
{

}

ThreadPool::~ThreadPool(void)
{

}

int 
ThreadPool::run_handler(void)
{
    while (!exit_) {
        os_sleep(1000);
        for (auto iter = idle_threads_.begin(); iter != idle_threads_.end(); ++iter) {
            int lifetime = iter->second->adjust_thread_life(1);
            if (lifetime <= 0) {
                iter->second->stop_handler(); // 设置线程关闭标志
                iter->second->resume(); // 唤醒空闲线程，执行线程关闭动作
            }
        }
    }
}

int ThreadPool::stop_handler(void)
{
    this->shutdown_all_threads();
    exit_ = true;
}

int 
ThreadPool::start_handler(void)
{
    exit_ = false;
}

int 
ThreadPool::add_task(Task &task)
{
    // 关闭线程池时，停止任务的添加
    if (exit_) {
        return 0;
    }
    return tasks_.push(task);
}

int 
ThreadPool::add_priority_task(Task &task)
{
    // 关闭线程池时，停止任务的添加
    if (exit_) {
        return 0;
    }
    return priority_tasks_.push(task);
}

int 
ThreadPool::get_task(Task &task)
{
    if (priority_tasks_.size() != 0) {
        return priority_tasks_.pop(task);
    } else {
        return tasks_.pop(task);
    }

    return 0;
}

int 
ThreadPool::set_threadpool_config(const ThreadPoolConfig &config)
{
    
    if (config.min_thread_num <= 0 || config.min_thread_num > MAX_THREADS_NUM) {
        LOG_WARNING("ThreadPool::set_threadpool_config(): min thread num is out of range --- %d", config.min_thread_num);
    } else {
        thread_pool_config_.min_thread_num =  config.min_thread_num;
    }

    if (config.max_thread_num <= 0 || config.max_thread_num > MAX_THREADS_NUM) {
        LOG_WARNING("ThreadPool::set_threadpool_config(): max thread num is out of range --- %d", config.max_thread_num);
    } else {
        thread_pool_config_.max_thread_num =  config.max_thread_num;
    }

    if (thread_pool_config_.idle_thread_life <= 0 || thread_pool_config_.idle_thread_life > MAX_THREAD_IDLE_LIFE) {
        LOG_WARNING("ThreadPool::set_threadpool_config(): thread life is out of range --- %d", thread_pool_config_.idle_thread_life);
    } else {
        thread_pool_config_.idle_thread_life = config.idle_thread_life;
    }

    if (config.threadpool_exit_action == WAIT_FOR_ALL_TASKS_FINISHED ) {
        thread_pool_config_.threadpool_exit_action = WAIT_FOR_ALL_TASKS_FINISHED;
    }

    return 0;
}

// pthread_cancel 可能会造成资源释放不正确
int 
ThreadPool::shutdown_all_threads(void)
{
    int action = thread_pool_config_.threadpool_exit_action;
    LOG_INFO("ThreadPool::shutdown_all_threads(): Action: %s", action == WAIT_FOR_ALL_TASKS_FINISHED ? "WAIT_FOR_ALL_TASKS_FINISHED":"SHUTDOWN_ALL_THREAD_IMMEDIATELY");
    
    if (action == WAIT_FOR_ALL_TASKS_FINISHED) {
        while (runing_threads_.size() > 0) {
            os_sleep(1000);
        }
    } else {

        for (auto iter = idle_threads_.begin(); iter != idle_threads_.end(); ++iter) {
            
        }
    }
}


}
