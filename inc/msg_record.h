#ifndef  __MSG_RECORD_H__
#define  __MSG_RECORD_H__

#include "basic_head.h"

namespace my_utils {
//////////////////////////////////////////////////////////////////////////////
enum InfoLevel {
    LOG_LEVEL_TRACE, // 跟踪程序运行流程
    LOG_LEVEL_DEBUG, // 调试用
    LOG_LEVEL_INFO,  // 打印信息
    LOG_LEVEL_WARN,  // 程序出现与预期不一致的错误(非致命性)
    LOG_LEVEL_ERROR, // 函数调用出现错误
    LOG_LEVEL_FATAL  // 程序无法继续运行
};

struct MsgContent {
    InfoLevel info_level;
    int which_line;
    string when;
    string which_file;
    string msg_info;
};

/////////////////////////////////////////////////////
// 默认标准输出函数
int output_to_stdout(const string &msg);

// 默认标准出错函数
int output_to_stderr(const string &msg);

/////////////////////////////////////////////////////
// 如果是多线程使用同一个回调函数来写消息， 在函数中需要处理消息的同步写。
typedef int (*msg_to_stream_callback)(const string &);

class MsgRecord {
public:
    MsgRecord(void);
    virtual ~MsgRecord(void);

    virtual void print_msg(InfoLevel level, int line, string file_name, const char *format, ...);
    virtual string get_msg_attr(InfoLevel level, int line, string file_name, const char *format, ...);
    virtual string assemble_msg(void);
    string level_convert(enum InfoLevel level);

    void set_stream_func(InfoLevel level, msg_to_stream_callback func);
    msg_to_stream_callback get_stream_func(InfoLevel level);

private:
    // 消息缓存
    vector<MsgContent> msg_info_;
    // 输出函数变量 
    msg_to_stream_callback msg_to_stream_trace_;
    msg_to_stream_callback msg_to_stream_debug_;
    msg_to_stream_callback msg_to_stream_info_;
    msg_to_stream_callback msg_to_stream_warn_;
    msg_to_stream_callback msg_to_stream_error_;
    msg_to_stream_callback msg_to_stream_fatal_;
};

#define LOG_TRACE(...)  this->print_msg(LOG_LEVEL_TRACE, __LINE__, __FILE__, __VA_ARGS__)
#define LOG_DEBUG(...)  this->print_msg(LOG_LEVEL_DEBUG, __LINE__, __FILE__, __VA_ARGS__)
#define LOG_INFO(...)  this->print_msg(LOG_LEVEL_INFO, __LINE__, __FILE__, __VA_ARGS__)
#define LOG_WARNING(...)  this->print_msg(LOG_LEVEL_WARN, __LINE__, __FILE__, __VA_ARGS__)
#define LOG_ERROR(...)  this->print_msg(LOG_LEVEL_ERROR, __LINE__, __FILE__, __VA_ARGS__)
#define LOG_FATAL(...)  this->print_msg(LOG_LEVEL_FATAL, __LINE__, __FILE__, __VA_ARGS__)

#define GET_MSG(...)  this->get_msg_attr(LOG_LEVEL_INFO, __LINE__, __FILE__, __VA_ARGS__)
}

#endif