#ifndef __SYSTEM_UTILS__
#define __SYSTEM_UTILS__

#include "basic_head.h"
#include "msg_record.h"

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

    virtual int get_errno(void) { return errno_;};
private:
    int errno_;
#ifdef __RJF_LINUX__
    pthread_mutex_t mutex_;
#endif
};

//////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////// 线程、线程池 ///////////////////////////////////////////
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

// thread_pool 的工作线程。
// 从 threadpool 工作队列中领任务，任务完成后暂停
// 有新任务分配时，启动继续执行任务
class ThreadPool;
class WorkThread : public Thread {
public:
    WorkThread(ThreadPool *thread_pool);
    virtual ~WorkThread(void);

    // 以下函数需要重载
    // 线程调用的主体
    virtual int run_handler(void);

    // 暂停线程
    virtual int pause(void);
    // 继续执行线程
    virtual int resume(void);
private:
    ThreadPool *thread_pool_;
};

class ThreadPool : public MsgRecord {
public:
    
private:
};

/////////////////////////////// 信号 //////////////////////////////////////////////////////


/////////////////////////////// 文件流 ////////////////////////////////////////////////////



/////////////////////////////// 网络套接字 /////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////

}

#endif