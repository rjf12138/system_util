#ifndef  __MSG_RECORD_H__
#define  __MSG_RECORD_H__

#include "basic_head.h"

namespace my_utils {
//////////////////////////////////////////////////////////////////////////////
enum InfoLevel {
    LOG_LEVEL_LOW, // 最低优先级
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_MAX // 最高优先级
};

struct MsgContent {
    InfoLevel info_level;
    int which_line;
    string when;
    string which_file;
    string msg_func;
    string msg_info;
};

/////////////////////////////////////////////////////
// 默认标准输出函数
extern int output_to_stdout(const string &msg);

// 默认标准出错函数
extern int output_to_stderr(const string &msg);

/////////////////////////////////////////////////////
// 设置到终端输出字符串颜色
enum StringColor {
    StringColor_None,       // 什么都不设置
    StringColor_Red,        // 红色
    StringColor_Green,      // 绿色
    StringColor_Yellow,     // 黄色
    StringColor_Blue,       // 蓝色
    StringColor_Magenta,    // 紫红色
    StringColor_Cyan        // 蓝绿色
};
// 返回带颜色设置的字符串
string set_string_color(const string &str, StringColor color);
/////////////////////////////////////////////////////
// 如果是多线程使用同一个回调函数来写消息， 在函数中需要处理消息的同步写。
typedef int (*msg_to_stream_callback)(const string &);

class MsgRecord {
public:
    MsgRecord(void);
    virtual ~MsgRecord(void);

    // 设置打印的等级，超过设定level的才会被输出
    void set_print_level(InfoLevel level);

    // 将消息通过设定的回调函数输出
    virtual void print_msg(InfoLevel level, int line, string file_name, string func, const char *format, ...);
    // 将消息以字符串方式返回
    virtual string get_msg(InfoLevel level, int line, string file_name, string func, const char *format, ...);

    // 组装消息
    void assemble_msg(ostringstream &ostr, const MsgContent &msg, bool is_color_enable = false);
    // 消息等级转为字符串
    string level_convert(InfoLevel level);

    // 设置消息回调函数
    void set_stream_func(InfoLevel level, msg_to_stream_callback func);
    // 获取消息回调函数
    msg_to_stream_callback get_stream_func(InfoLevel level);

public:
    // 用于非类里面的全局输出
    static MsgRecord g_log_msg;

private:
    InfoLevel print_level_; // 设置输出等级， 低于该等级的将不会被输出 

    // 输出函数变量 
    msg_to_stream_callback msg_to_stream_trace_;
    msg_to_stream_callback msg_to_stream_debug_;
    msg_to_stream_callback msg_to_stream_info_;
    msg_to_stream_callback msg_to_stream_warn_;
    msg_to_stream_callback msg_to_stream_error_;
    msg_to_stream_callback msg_to_stream_fatal_;
};

#define SET_PRINT_LEVEL(x)  this->set_print_level(x)
#define LOG_TRACE(...)      this->print_msg(LOG_LEVEL_TRACE, __LINE__, __FILE__, __FUNCTION__, __VA_ARGS__)
#define LOG_DEBUG(...)      this->print_msg(LOG_LEVEL_DEBUG, __LINE__, __FILE__, __FUNCTION__, __VA_ARGS__)
#define LOG_INFO(...)       this->print_msg(LOG_LEVEL_INFO, __LINE__, __FILE__, __FUNCTION__, __VA_ARGS__)
#define LOG_WARN(...)       this->print_msg(LOG_LEVEL_WARN, __LINE__, __FILE__, __FUNCTION__, __VA_ARGS__)
#define LOG_ERROR(...)      this->print_msg(LOG_LEVEL_ERROR, __LINE__, __FILE__, __FUNCTION__, __VA_ARGS__)
#define LOG_FATAL(...)      this->print_msg(LOG_LEVEL_FATAL, __LINE__, __FILE__, __FUNCTION__, __VA_ARGS__)
#define SET_CALLBACK(LEVEL, FUNC) this->set_stream_func(LEVEL, FUNC)

#define GET_MSG(...)  this->get_msg(LOG_LEVEL_INFO, __LINE__, __FILE__, __FUNCTION__, __VA_ARGS__)


#define SET_GLOBAL_PRINT_LEVEL(x) MsgRecord::g_log_msg.set_print_level(x)
#define LOG_GLOBAL_TRACE(...)  MsgRecord::g_log_msg.print_msg(LOG_LEVEL_TRACE, __LINE__, __FILE__, __FUNCTION__, __VA_ARGS__)
#define LOG_GLOBAL_DEBUG(...)  MsgRecord::g_log_msg.print_msg(LOG_LEVEL_DEBUG, __LINE__, __FILE__, __FUNCTION__, __VA_ARGS__)
#define LOG_GLOBAL_INFO(...)   MsgRecord::g_log_msg.print_msg(LOG_LEVEL_INFO, __LINE__, __FILE__, __FUNCTION__, __VA_ARGS__)
#define LOG_GLOBAL_WARN(...)   MsgRecord::g_log_msg.print_msg(LOG_LEVEL_WARN, __LINE__, __FILE__, __FUNCTION__, __VA_ARGS__)
#define LOG_GLOBAL_ERROR(...)  MsgRecord::g_log_msg.print_msg(LOG_LEVEL_ERROR, __LINE__, __FILE__, __FUNCTION__, __VA_ARGS__)
#define LOG_GLOBAL_FATAL(...)  MsgRecord::g_log_msg.print_msg(LOG_LEVEL_FATAL, __LINE__, __FILE__, __FUNCTION__, __VA_ARGS__)
#define SET_GLOBAL_CALLBACK(LEVEL, FUNC)  MsgRecord::g_log_msg.set_stream_func(LEVEL, FUNC)

}

#endif