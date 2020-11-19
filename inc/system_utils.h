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

// 休眠时间,单位：毫秒 
int os_sleep(int millisecond);

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
    void *thread_arg;
    void *exit_arg;
    thread_callback work_func;
    thread_callback exit_task;
};

// thread_pool 的工作线程。
// 从 threadpool 工作队列中领任务，任务完成后暂停
// 有新任务分配时，启动继续执行任务
enum ThreadState {
    WorkThread_RUNNING,
    WorkThread_WAITING,
    WorkThread_EXIT,
};

class ThreadPool;
class WorkThread : public Thread {
    friend ThreadPool;
public:
    WorkThread(ThreadPool *thread_pool, int idle_life = 30);
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
    virtual int adjust_thread_life(int des_time) {remain_life_ -= des_time; return remain_life_;}

private:
    int idle_life_; // 单位：秒
    int remain_life_;
    int state_;
    int64_t thread_id_;

    Task task_;
    Mutex mutex_;
    ThreadPool *thread_pool_;
    
#ifdef __RJF_LINUX__
    pthread_cond_t thread_cond_;
#endif
};

// 添加时间轮来防止空闲的线程过多
#define MAX_THREADS_NUM         500     // 最多 500 个线程
#define MAX_THREAD_IDLE_LIFE    1800    // 最长半小时
enum ThreadPoolExitAction {
    UNKNOWN_ACTION,                     // 未知行为
    WAIT_FOR_ALL_TASKS_FINISHED,        // 等待所有任务都完成
    SHUTDOWN_ALL_THREAD_IMMEDIATELY,    // 立即关闭所有线程
};

struct ThreadPoolConfig {
    int min_thread_num;
    int max_thread_num;
    int idle_thread_life;
    int threadpool_exit_action; // 默认时强制关闭所有线程
};

class ThreadPool : public Thread {
    friend WorkThread;
public:
    ThreadPool(void);
    virtual ~ThreadPool(void);

    // 以下函数需要重载
    // 线程调用的主体
    virtual int run_handler(void);
    virtual int stop_handler(void);
    virtual int start_handler(void);

    // 添加普通任务
    int add_task(Task &task);
    // 任务将被优先执行
    int add_priority_task(Task &task);

    // 设置最小的线程数量，当线程数量等于它时，线程即使超出它寿命依旧不杀死
    int set_threadpool_config(const ThreadPoolConfig &config);

private:
    // 关闭线程池中的所有线程
    int shutdown_all_threads(void);

    // 获取任务，优先队列中的任务先被取出,有任务返回大于0，否则返回等于0
    // 除了工作线程之外，其他任何代码都不要去调用该函数，否则会导致的任务丢失
    int get_task(Task &task);

    // 移除已经关闭的工作线程信息
    // 注意：使用时要确保线程已经结束了
    int remove_thread(int64_t thread_id);

    // 工作线程初始化以及动态调整线程数量
    // 任务的分配
    int manage_work_threads(bool is_init);

private:
    void thread_move_to_idle_map(int64_t thread_id);
    void thread_move_to_running_map(int64_t thread_id);
    
private:
    bool exit_;
    ThreadPoolConfig thread_pool_config_;// 多余的空闲线程会被杀死，直到数量达到最小允许的线程数为止。单位：秒

    Mutex mutex_;

    Queue<Task> tasks_;   // 普通任务队列
    Queue<Task> priority_tasks_; // 优先任务队列
    map<int64_t, WorkThread*> runing_threads_; // 运行中的线程列表
    map<int64_t, WorkThread*> idle_threads_; // 空闲的线程列表
};

/////////////////////////////// 信号 //////////////////////////////////////////////////////


/////////////////////////////// 文件流 ////////////////////////////////////////////////////



/////////////////////////////// 网络套接字 /////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////

}

#endif