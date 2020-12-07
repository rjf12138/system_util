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
        return -1;
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
        return -1;
    }
#endif

    return 0;
}

////////////////////////////////////// WorkThread ///////////////////////////////////////
WorkThread::WorkThread(ThreadPool *thread_pool, int idle_life)
: idle_life_(idle_life), remain_life_(idle_life), thread_pool_(thread_pool)
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
    while (state_ != WorkThread_EXIT) {
        mutex_.lock();
        while (state_ == WorkThread_WAITING) {
            thread_cond_wait();
        }
        mutex_.unlock();
        if (state_ == WorkThread_RUNNING) {
            if (thread_pool_->get_task(task_) > 0) {
                LOG_DEBUG("RUNNING THREAD");
                task_.state = THREAD_TASK_HANDLE;
                task_.work_func(task_.thread_arg);
                task_.state = THREAD_TASK_COMPLETE;
            }
        }

        this->pause();
    }
    // 从线程池中删除关闭的线程信息
    thread_pool_->remove_thread((int64_t)this);

    return 0;
}

int 
WorkThread::stop_handler(void)
{
    // 设置线程为退出状态
    int old_state = state_;
    state_ = WorkThread_EXIT;
    // 运行中的线程，调用由任务发布者设置的任务退出函数
    if (old_state == WorkThread_RUNNING) {
        if (task_.exit_task != nullptr) {
            task_.exit_task(task_.exit_arg);
        }
    } else if (old_state == WorkThread_WAITING) { // 唤醒空闲线程，然后走结束线程的流程
        this->resume();
    }

    return 0;
}

int 
WorkThread::start_handler(void)
{
    state_ = WorkThread_WAITING;

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
        thread_pool_->thread_move_to_idle_map((int64_t)this);
    }

    return 0;
}

int 
WorkThread::resume(void)
{
    if (state_ == WorkThread_WAITING) {
        state_ = WorkThread_RUNNING;
        thread_pool_->thread_move_to_running_map((int64_t)this);
    }

    mutex_.lock();
    thread_cond_signal();
    mutex_.unlock();

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////
ThreadPool::ThreadPool(void)
: exit_(false)
{
    thread_pool_config_.min_thread_num = 5;
    thread_pool_config_.max_thread_num = 10;
    thread_pool_config_.idle_thread_life = 30;
    thread_pool_config_.threadpool_exit_action = SHUTDOWN_ALL_THREAD_IMMEDIATELY;
    this->manage_work_threads(true);
}

ThreadPool::~ThreadPool(void)
{

}

int 
ThreadPool::run_handler(void)
{
    while (!exit_) {
        os_sleep(1000);
        this->manage_work_threads(false);
    }

    return 0;
}

int ThreadPool::stop_handler(void)
{
    exit_ = true;
    return this->shutdown_all_threads();
}

int 
ThreadPool::start_handler(void)
{
    exit_ = false;

    return 0;
}
int i = 0;
int 
ThreadPool::add_task(Task &task)
{
    // 关闭线程池时，停止任务的添加
    if (exit_) {
        return 0;
    }
    task_mutex_.lock();
    int ret = tasks_.push(task);
    LOG_DEBUG("add_task---locked");
    ++i;
    task_mutex_.unlock();
    LOG_DEBUG("add_task---unlocked");
    --i;

    return ret;
}

int 
ThreadPool::add_priority_task(Task &task)
{
    // 关闭线程池时，停止任务的添加
    if (exit_) {
        return 0;
    }
    task_mutex_.lock();
    int ret = priority_tasks_.push(task);
    LOG_DEBUG("add_priority_task---locked");
    task_mutex_.unlock();
    LOG_DEBUG("add_priority_task---unlocked");

    return ret;
}

int 
ThreadPool::get_task(Task &task)
{
    LOG_DEBUG("get_task---wait lock: %s", task_mutex_.get_state()?"true":"false");
    task_mutex_.lock();
    if (priority_tasks_.size() != 0) {
        return priority_tasks_.pop(task);
    } else {
        return tasks_.pop(task);
    }
    LOG_DEBUG("get_task---locked");
    ++i;
    task_mutex_.unlock();
    LOG_DEBUG("get_task---unlocked");
    --i;

    return 0;
}

int 
ThreadPool::remove_thread(int64_t thread_id)
{
    thread_mutex_.lock();
    auto remove_iter = idle_threads_.find(thread_id);
    if (remove_iter != idle_threads_.end()) {
        if (remove_iter->second != nullptr) {
            delete remove_iter->second;
        }
        idle_threads_.erase(remove_iter);
    }

    remove_iter = runing_threads_.find(thread_id);
    if (remove_iter != runing_threads_.end()) {
        if (remove_iter->second != nullptr) {
            delete remove_iter->second;
        }
        runing_threads_.erase(remove_iter);
    }
    thread_mutex_.unlock();

    return 0;
}

int 
ThreadPool::manage_work_threads(bool is_init)
{
    if (is_init) {
        for (std::size_t i = 0; i < thread_pool_config_.min_thread_num; ++i) {
            WorkThread *work_thread = new WorkThread(this);
            work_thread->init();
            idle_threads_[(int64_t)work_thread] = work_thread;
        }

        return 0;
    }

    if (tasks_.size() > 0 ) {
        int awake_threads = tasks_.size() / 2 + 1;
        for (int i = 0; i < awake_threads; ++i) {
            if (idle_threads_.size() > 0) {
                idle_threads_.begin()->second->resume();
            } else {
                if (runing_threads_.size() < thread_pool_config_.max_thread_num) {
                    WorkThread *work_thread = new WorkThread(this);
                    work_thread->init();
                    idle_threads_[(int64_t)work_thread] = work_thread;
                    --i;
                    continue;
                } else {
                    // 可分配的线程数已经达到最大
                    LOG_WARN("run out of threads: running threads: %d, max threads: %d", runing_threads_.size(), thread_pool_config_.max_thread_num);
                    break;
                }
            }
        }
    }

    for (auto iter = idle_threads_.begin(); iter != idle_threads_.end();) {
        auto stop_iter = iter++; // stop_handler 可能会改变idle_threads从而影响到当前的iter,先提前自增

        int lifetime = stop_iter->second->adjust_thread_life(1);
        if (lifetime <= 0) {
            stop_iter->second->stop_handler(); // 关闭多余的空闲线程
        }
    }

    return 0;
}

void 
ThreadPool::thread_move_to_idle_map(int64_t thread_id)
{
    thread_mutex_.lock();
    auto iter = runing_threads_.find(thread_id);
    if (iter == runing_threads_.end()) {
        LOG_WARN("Can't find thread(thread_id: %ld) at runing_threads", thread_id);
        thread_mutex_.unlock();
        return ;
    }

    if (idle_threads_.find(thread_id) != idle_threads_.end()) {
        LOG_WARN("There is exists a thread(thread_id: %ld) at idle_threads", thread_id);
        thread_mutex_.unlock();
        return ;
    }

    idle_threads_[thread_id] = iter->second;
    runing_threads_.erase(iter);

    thread_mutex_.unlock();

    return ;
}

void 
ThreadPool::thread_move_to_running_map(int64_t thread_id)
{
    thread_mutex_.lock();
    auto iter = idle_threads_.find(thread_id);
    if (iter == idle_threads_.end()) {
        LOG_WARN("Can't find thread(thread_id: %ld) at idle_threads", thread_id);
        thread_mutex_.unlock();
        return ;
    }

    if (runing_threads_.find(thread_id) != runing_threads_.end()) {
        LOG_WARN("There is exists a thread(thread_id: %ld) at runing_threads", thread_id);
        thread_mutex_.unlock();
        return ;
    }

    runing_threads_[thread_id] = iter->second;
    idle_threads_.erase(iter);

    thread_mutex_.unlock();

    return ;
}

int 
ThreadPool::set_threadpool_config(const ThreadPoolConfig &config)
{

    if (config.min_thread_num < 0 || config.min_thread_num > MAX_THREADS_NUM) {
        LOG_WARN("min thread num is out of range --- %d", config.min_thread_num);
    } else {
        thread_pool_config_.min_thread_num =  config.min_thread_num;
    }

    if (config.max_thread_num < config.min_thread_num || config.max_thread_num > MAX_THREADS_NUM) {
        LOG_WARN("max thread num is out of range --- %d", config.max_thread_num);
    } else {
        thread_pool_config_.max_thread_num =  config.max_thread_num;
    }

    if (thread_pool_config_.idle_thread_life <= 0 || thread_pool_config_.idle_thread_life > MAX_THREAD_IDLE_LIFE) {
        LOG_WARN("thread life is out of range --- %d", thread_pool_config_.idle_thread_life);
    } else {
        thread_pool_config_.idle_thread_life = config.idle_thread_life;
    }

    if (config.threadpool_exit_action == WAIT_FOR_ALL_TASKS_FINISHED ) {
        thread_pool_config_.threadpool_exit_action = WAIT_FOR_ALL_TASKS_FINISHED;
    }

    return 0;
}

int 
ThreadPool::shutdown_all_threads(void)
{
    int action = thread_pool_config_.threadpool_exit_action;
    LOG_INFO("Action: %s", action == WAIT_FOR_ALL_TASKS_FINISHED ? "WAIT_FOR_ALL_TASKS_FINISHED":"SHUTDOWN_ALL_THREAD_IMMEDIATELY");
    
    if (action == WAIT_FOR_ALL_TASKS_FINISHED) {
        // 等待任务完成
        while (runing_threads_.size() > 0) {
            LOG_INFO("Wait for task finish, Working threads num: %d", runing_threads_.size());
            os_sleep(1000);
        }
    } else {
        // 停止所有正在处理任务的线程
        for (auto iter = runing_threads_.begin(); iter != runing_threads_.end();) {
            auto stop_iter = iter++;
            stop_iter->second->stop_handler();
        }
    }

    while (runing_threads_.size() > 0) {
        LOG_ERROR("shutdown threads error: %d threads still running", runing_threads_.size());
    }

    for (auto iter = idle_threads_.begin(); iter != idle_threads_.end(); ) {
        auto stop_iter = iter++;
        stop_iter->second->stop_handler();
    }

    return 0;
}


}
