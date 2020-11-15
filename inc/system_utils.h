#ifndef __SYSTEM_UTILS__
#define __SYSTEM_UTILS__

#include "basic_head.h"
#include "msg_record.h"
#include "data_structure.h"

using namespace my_utils;

namespace system_utils {

// 设置所有系统函数的回调输出
extern msg_to_stream_callback g_msg_to_stream_trace;
extern msg_to_stream_callback g_msg_to_stream_debug;
extern msg_to_stream_callback g_msg_to_stream_info;
extern msg_to_stream_callback g_msg_to_stream_warn;
extern msg_to_stream_callback g_msg_to_stream_error;
extern msg_to_stream_callback g_msg_to_stream_fatal;

extern void set_systemcall_message_output_callback(InfoLevel level, msg_to_stream_callback func);

/*
* 所有的类成员函数成功了返回0， 失败返回非0值
*/
/////////////////////////////// 互斥量 /////////////////////////////////////////////
class Mutex : public MsgRecord {
public:
    Mutex(void);
    virtual ~Mutex(void);

    virtual int lock(void);
    virtual int trylock(void);
    virtual int unlock(void);

    virtual int get_errno(void) { return errno_;}
#ifdef __RJF_LINUX__
    pthread_mutex_t* get_mutex(void) {return &mutex_;}
#endif
private:
    int errno_;
#ifdef __RJF_LINUX__
    pthread_mutex_t mutex_;
#endif
};

//////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////// 线程、线程池 ///////////////////////////////////////////
// 线程回调函数
#ifdef __RJF_LINUX__
    typedef void *(*thread_callback)(void*);
#endif

// 通用线程类，直接继承它，设置需要重载的函数
class Thread : public MsgRecord {
public:
    Thread(void);
    virtual ~Thread(void);

    static void* create_func(void *arg);

    virtual int init(void);
    virtual int wait_thread(void);

    // 以下函数需要重载
    // 线程调用的主体
    virtual int run_handler(void);
    // 设置结束标志，调用完后线程结束运行
    virtual int stop_handler(void);
    // 设置开始标识，设置完后线程可以运行
    virtual int start_handler(void);

private:
#ifdef __RJF_LINUX__
    pthread_t thread_id_;
#endif
};

///////////////////////////////////////// Thread Pool /////////////////////////////////
enum TaskWorkState {
    THREAD_TASK_WAIT,
    THREAD_TASK_HANDLE,
    THREAD_TASK_COMPLETE,
    THREAD_TASK_ERROR,
};

struct Task {
    int state;
    void *arg;
    thread_callback work_func;
};

// thread_pool 的工作线程。
// 从 threadpool 工作队列中领任务，任务完成后暂停
// 有新任务分配时，启动继续执行任务
enum ThreadState {
    WorkThread_RUNNING,
    WorkThread_WAITING,
};

class ThreadPool;
class WorkThread : public Thread {
    friend ThreadPool;
public:
    WorkThread(ThreadPool *thread_pool);
    virtual ~WorkThread(void);

    // 以下函数需要重载
    // 线程调用的主体
    virtual int run_handler(void);
    virtual int stop_handler(void);
    virtual int start_handler(void);

    void thread_cond_wait(void);
    void thread_cond_signal(void);

    // 暂停线程
    virtual int pause(void);
    // 继续执行线程
    virtual int resume(void);

    virtual int64_t get_thread_id(void) const {return thread_id_;}
    virtual int get_current_state(void) const {return state_;}

private:
    bool exit_;
    int max_life_; // 单位：秒
    int state_;
    int64_t thread_id_;

    Mutex mutex_;
    ThreadPool *thread_pool_;
    
#ifdef __RJF_LINUX__
    pthread_cond_t thread_cond_;
#endif
};

// 添加时间轮来防止空闲的线程过多
class ThreadPool : public MsgRecord {
public:
    ThreadPool(void);
    ThreadPool(int thread_num);
    virtual ~ThreadPool(void);

    int add_task(Task &task);
    // 任务将被优先执行
    int add_priority_task(Task &task);
    // 获取任务，优先队列中的任务先被取出,有任务返回大于0，否则返回等于0
    int get_task(Task &task);

    // 设置最小的线程数量，当线程数量等于它时，线程即使超出它寿命依旧不杀死
    int set_min_thread_number(int thread_num);
    int set_max_thread_number(int thread_num);
    int set_thread_life(int life);

private:
    void thread_move_to_idle_map(int64_t thread_id);
    void thread_move_to_idle_map(int64_t thread_id);
    
private:
    int min_thread_num_;
    int max_thread_num_;
    int thread_life_;

    Mutex mutex_;

    Queue<Task> tasks_;
    Queue<Task> priority_tasks_;
    map<int64_t, WorkThread*> runing_threads_;
    map<int64_t, WorkThread*> idle_threads_;
};

/////////////////////////////// 信号 //////////////////////////////////////////////////////


/////////////////////////////// 文件流 ////////////////////////////////////////////////////



/////////////////////////////// 网络套接字 /////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////

}

#endif