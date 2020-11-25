#include "byte_buffer.h"
#include "system_utils.h"

using namespace system_utils;

void *echo_handler(void *arg);
void *echo_exit(void *arg);

int main(void)
{
    ByteBuffer buff;
    string str;

    std::size_t min_thread = 59;
    std::size_t max_thread = 60;
    ThreadPoolConfig config = {min_thread, max_thread, 30, SHUTDOWN_ALL_THREAD_IMMEDIATELY};
    ThreadPool pool;
    pool.init();
    pool.set_threadpool_config(config);

    while (true) {
        Socket *cli = new Socket("172.16.8.1", 12138);
        Task task;
        task.exit_arg = cli;
        task.thread_arg = cli;
        task.work_func = echo_handler;
        task.exit_task = echo_exit;

        pool.add_task(task);
    }

    return 0;
}

void *echo_handler(void *arg)
{
    if (arg == nullptr) {
        return nullptr;
    }

    Socket *cli_info = (Socket*)arg;
    string cli_ip;
    short cli_port;
    cli_info->get_ip_info(cli_ip, cli_port);

    if (cli_info->connect() != 0) {
        printf("Client: %s:%d connect failed!\n", cli_ip.c_str(), cli_port);
        return nullptr;
    }
    printf("Client: %s:%d connect successed！\n", cli_ip.c_str(), cli_port);

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
        if (recv_data != data) {
            printf("Client: %s:%d exit error!\n", cli_ip.c_str(), cli_port);
            break;
        }
    }

    printf("Client: %s:%d exit successed!\n", cli_ip.c_str(), cli_port);
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