#include "byte_buffer.h"
#include "system_utils.h"

using namespace system_utils;

void *echo_handler(void *arg);
void *echo_exit(void *arg);

ThreadPool pool;
bool server_exit = false;

// 打断阻塞的accept函数
static jmp_buf jmpbuf; 
void accept_exit(int sig);

// 通知退出程序
void sig_int(int sig);

int main(void)
{
    if (signal(SIGINT, sig_int) == SIG_ERR) {
        LOG_GLOBAL_DEBUG("signal error!");
        return 0;
    }

    if (signal(SIGIO, accept_exit) == SIG_ERR) {
        LOG_GLOBAL_DEBUG("signal error!");
        return 0;
    }

    ByteBuffer buff;
    string str;

    std::size_t min_thread = 250;
    std::size_t max_thread = 260;
    ThreadPoolConfig config = {min_thread, max_thread, 30, SHUTDOWN_ALL_THREAD_IMMEDIATELY};
    pool.init();
    pool.set_threadpool_config(config);

    Socket echo_server("127.0.0.1", 12138);
    echo_server.listen();

    int cli_socket;
    struct sockaddr_in cliaddr;
    socklen_t size = sizeof(cliaddr);

    if(setjmp(jmpbuf)) {
        pool.stop_handler();
    } 

    while (!server_exit) {
        int ret = echo_server.accept(cli_socket, (sockaddr*)&cliaddr, &size);
        if (ret != 0) {
            break;
        }
        LOG_GLOBAL_DEBUG("accept socket!");
        Socket *cli = new Socket;
        cli->set_socket(cli_socket, &cliaddr, &size);
        Task task;
        task.exit_arg = cli;
        task.thread_arg = cli;
        task.work_func = echo_handler;
        task.exit_task = echo_exit;

        pool.add_task(task);
    }

    return 0;
}

void sig_int(int sig)
{
    LOG_GLOBAL_DEBUG("Exit server!");
    server_exit = true;
    kill(getpid(), SIGIO);
}

void accept_exit(int sig)
{
    longjmp(jmpbuf, 1);
}

void *echo_handler(void *arg)
{
    if (arg == nullptr) {
        return nullptr;
    }

    Socket *cli_info = (Socket*)arg;
    string cli_ip;
    uint16_t cli_port;
    cli_info->get_ip_info(cli_ip, cli_port);
    LOG_GLOBAL_DEBUG("Client: %s:%d connect！", cli_ip.c_str(), cli_port);

    ByteBuffer buff;
    while (cli_info->get_socket_state()) {
        int ret = cli_info->recv(buff, 2048, 0);
        if (ret == 0) {
            string str;
            buff.read_string(str);
            LOG_GLOBAL_DEBUG("Client (%s:%d): %s", cli_ip.c_str(), cli_port, str.c_str());
            buff.write_string(str);
            cli_info->send(buff, ret, 0);
            
            if (str == "quit") {
                break;
            }
        } else {
            LOG_GLOBAL_DEBUG("Client (%s:%d): Recv error break.", cli_ip.c_str(), cli_port);
            break;
        }
    }
    // 给2s时间把数据发送到客户端在关闭
    os_sleep(2000);
    LOG_GLOBAL_DEBUG("Client: %s:%d exit!", cli_ip.c_str(), cli_port);
    delete cli_info;
    cli_info = nullptr;

    return nullptr;
}

void *echo_exit(void *arg)
{
    if (arg == nullptr) {
        return nullptr;
    }

    Socket *cli_info = (Socket*)arg;
    cli_info->close();

    return nullptr;
}