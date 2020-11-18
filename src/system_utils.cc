#include "system_utils.h"


// 放不是实现单一功能的函数
namespace system_utils {

// 设置system_utils内所有函数/类成员的消息回调
msg_to_stream_callback g_msg_to_stream_trace = output_to_stdout;
msg_to_stream_callback g_msg_to_stream_debug = output_to_stdout;
msg_to_stream_callback g_msg_to_stream_info = output_to_stdout;
msg_to_stream_callback g_msg_to_stream_warn = output_to_stderr;
msg_to_stream_callback g_msg_to_stream_error = output_to_stderr;
msg_to_stream_callback g_msg_to_stream_fatal = output_to_stderr;

void set_systemcall_message_output_callback(InfoLevel level, msg_to_stream_callback func)
{
     switch (level) {
        case LOG_LEVEL_TRACE:
            g_msg_to_stream_trace = func; break;
        case LOG_LEVEL_DEBUG:
            g_msg_to_stream_debug = func; break;
        case LOG_LEVEL_INFO:
            g_msg_to_stream_info = func; break;
        case LOG_LEVEL_WARN:
            g_msg_to_stream_warn = func; break;
        case LOG_LEVEL_ERROR:
            g_msg_to_stream_error = func; break;
        case LOG_LEVEL_FATAL:
            g_msg_to_stream_fatal = func; break;
        default: 
        {
            output_to_stderr("MsgRecord::set_stream_func: Unknown option!");
        } break;
    }
}


int os_sleep(int millisecond)
{
#ifdef __RJF_LINUX__
    return usleep(millisecond);
#endif
}

}