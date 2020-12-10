#ifndef __SYSTEM_UTILS__
#define __SYSTEM_UTILS__

#include "basic_head.h"
#include "msg_record.h"
#include "data_structure/data_structure.h"
#include "data_structure/queue.h"
#include "byte_buffer.h"

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

    virtual bool get_state(void) {return state_;}
    virtual int get_errno(void) { return errno_;}
#ifdef __RJF_LINUX__
    pthread_mutex_t* get_mutex(void) {return &mutex_;}
#endif
private:
    bool state_;
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
    virtual int idle_timeout(void);
    virtual int reset_idle_life(void);

private:
    time_t idle_life_; // 单位：秒
    time_t start_idle_life_;
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
    std::size_t min_thread_num;
    std::size_t max_thread_num;
    int idle_thread_life;
    ThreadPoolExitAction threadpool_exit_action; // 默认时强制关闭所有线程
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
    // 打印线程池信息
    string info(void);
    // 获取线程池状态
    ThreadState get_state(void) {return state_;}

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
    int thread_move_to_idle_map(int64_t thread_id);
    int thread_move_to_running_map(int64_t thread_id);
    
private:
    bool exit_;
    ThreadState state_;
    ThreadPoolConfig thread_pool_config_;// 多余的空闲线程会被杀死，直到数量达到最小允许的线程数为止。单位：秒

    Mutex thread_mutex_;
    Mutex task_mutex_;

    Queue<Task> tasks_;   // 普通任务队列
    Queue<Task> priority_tasks_; // 优先任务队列
    map<int64_t, WorkThread*> runing_threads_; // 运行中的线程列表
    map<int64_t, WorkThread*> idle_threads_; // 空闲的线程列表
};

/////////////////////////////// 文件流 ////////////////////////////////////////////////////
// 修改一下成功返回0，失败返回非0
#define DEFAULT_OPEN_FLAG   O_RDWR
#define DEFAULT_FILE_RIGHT  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

class Stream : public MsgRecord {
public:
    Stream(void);
    ~Stream(void);

    // 根据文件路径打开文件
    int open(const string file_path, int flag = DEFAULT_OPEN_FLAG, int file_right = DEFAULT_FILE_RIGHT);
    // 根据文件描述符打开文件
    // 默认退出时不关闭文件描述符
    int set_fd(int fd, bool open_when_exit = true);
    int get_fd(void) const {return fd_;};

    // 返回文件信息
    int fileinfo(struct stat &file_info);
    // 返回文件的大小
    off_t file_size() {return file_info_.st_size;}
    // 关闭文件
    int close_file(void);
    // 检查文件描述符
    int check_fd(int fd);
    // 清空文件
    int clear_file(void);
    
    // 设置文件偏移
    off_t seek(off_t offset, int whence);
    // 返回当前文件偏移
    off_t curr_pos(void);

    // 从当前位置读取任意数据
    ssize_t read(ByteBuffer &buff, size_t buf_size);
    // 从某一位置读取数据
    ssize_t read_from_pos(ByteBuffer &buff, size_t buf_size, off_t pos, int whence);

    // 写数据
    ssize_t write(ByteBuffer &buff, size_t buf_size);
    // 从某一位置写数据
    ssize_t write_to_pos(ByteBuffer &buff, size_t buf_size ,off_t pos, int whence);

    // 格式化读/写到缓存中
    static ssize_t read_buf_fmt(ByteBuffer &buff, const char *fmt, ...);
    static ssize_t write_buf_fmt(ByteBuffer &buff, const char *fmt, ...);

    // 格式化写到文件中
    ssize_t write_file_fmt(const char *fmt, ...);

private:
    int fd_;
    bool open_on_exit_;
    bool file_open_flag_;
    unsigned max_lines_;
    struct stat file_info_;
};


/////////////////////////////// 网络套接字 /////////////////////////////////////////////////
// IPv4: 只支持TCP
class Socket : public MsgRecord {
public:
    Socket(void);
    Socket(string ip, uint16_t port);
    ~Socket();

    // 设置不是由本类所创建的套接字
    int set_socket(int clisock, struct sockaddr_in *cliaddr = nullptr, socklen_t *addrlen = nullptr);
    // ip,port可以是本地的用于创建服务端程序，也可以是客户端连接到服务端IP和端口
    int create_socket(string ip, uint16_t port);

    // 这里listen()合并了 bind()和listen
    int listen(void);
    // 客户端连接到服务端
    int connect(void);
    // 由TCP服务器调用
    // clisock: 返回的客户端socket
    // cliaddr: 返回的客户端IP信息
    // addrlen: 设置cliaddr 结构的大小
    int accept(int &clisock, struct sockaddr *cliaddr = nullptr, socklen_t *addrlen = nullptr);
    int recv(ByteBuffer &buff, int buff_size, int flags);
    int send(ByteBuffer &buff, int buff_size, int flags);

    // 配置socket
    // 设置成非阻塞
    int setnonblocking(void);
    // 设置成地址复用
    int set_reuse_addr(void);
    
    // 获取socket信息
    int get_socket(void);
    int get_ip_info(string &ip, uint16_t &port);
    bool get_socket_state(void) const {return is_enable_;}

    // 关闭套接字
    int close(void);
    // 禁用套接字I/O
    // how:的选项
    // SHUT_WR(关闭写端)
    // SHUT_RD(关闭读端)
    // SHUT_RDWR(关闭读和写)
    int shutdown(int how);
private:
    bool is_enable_;
    int socket_;

    uint16_t port_;
    string ip_;

    struct sockaddr_in addr_;
};
///////////////////////////////////////////////////////////////////////////////////////////

}

#endif