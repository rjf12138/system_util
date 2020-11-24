#include "byte_buffer.h"
#include "system_utils.h"

using namespace system_utils;

void *echo_handler(void *arg);
void *echo_exit(void *arg);

int main(void)
{
    ByteBuffer buff;
    string str;

    ThreadPool pool;
    pool.init();

    Socket echo_server("127.0.0.1", 12138);
    echo_server.listen();

    int cli_socket;
    struct sockaddr_in cliaddr;
    socklen_t size = sizeof(cliaddr);

    while (true) {
        int ret = echo_server.accept(cli_socket, (sockaddr*)&cliaddr, &size);
        if (ret != 0) {
            break;
        }
        Socket *cli = new Socket;
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
    printf("Client: %s:%d connect！\n", cli_ip.c_str(), cli_port);

    ByteBuffer buff;
    while (cli_info->get_socket_state()) {
        int ret = cli_info->recv(buff, 2048, 0);
        if (ret > 0) {
            string str;
            buff.read_string(str);

            if (str == "quit") {
                break;
            }

            cli_info->send(buff, ret, 0);
        }
    }

    printf("Client: %s:%d exit!\n", cli_ip.c_str(), cli_port);
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