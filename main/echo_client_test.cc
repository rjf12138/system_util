#include "byte_buffer.h"
#include "system_utils.h"

using namespace system_utils;

void *echo_handler(void *arg);
void *echo_exit(void *arg);

ThreadPool pool;

void sig_int(int sig);

int main(void)
{
    if (signal(SIGINT, sig_int) == SIG_ERR) {
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

    while(true) {
        for (std::size_t i = 0; i < min_thread; ++i) {
            Socket *cli = new Socket("127.0.0.1", 12138);
            Task task;
            task.exit_arg = cli;
            task.thread_arg = cli;
            task.work_func = echo_handler;
            task.exit_task = echo_exit;

            pool.add_task(task);
        }
    }
    pool.wait_thread();

    return 0;
}

void sig_int(int sig)
{
    LOG_GLOBAL_DEBUG("Exit client!");
    pool.stop_handler();
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

    if (cli_info->connect() != 0) {
        LOG_GLOBAL_DEBUG("Client: %s:%d connect failed!", cli_ip.c_str(), cli_port);
        return nullptr;
    }
    LOG_GLOBAL_DEBUG("Client: %s:%d connect successed!", cli_ip.c_str(), cli_port);

    string data = "{\"data\": \"client\", \"num\":12138}";
    ByteBuffer buff;
    

    for (int i = 0; i <= 10 && cli_info->get_socket_state(); ++i) {
        buff.clear();
        if (i == 10) {
            buff.write_string("quit");
        } else {
            buff.write_string(data);
        }

        int ret = cli_info->send(buff, data.size(), 0);
        cli_info->recv(buff, ret, 0);
        string recv_data;
        buff.read_string(recv_data);
        if (recv_data != data && recv_data != "quit") {
            LOG_GLOBAL_ERROR("Client: %s:%d exit error!", cli_ip.c_str(), cli_port);
            break;
        } else {
            LOG_GLOBAL_DEBUG("Recv from server: %s", recv_data.c_str());
            if (recv_data == "quit") {
                break;
            }
        }

        os_sleep(100);
    }

    LOG_GLOBAL_DEBUG("Client: %s:%d exit successed!", cli_ip.c_str(), cli_port);
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