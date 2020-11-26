#include "system_utils.h"

namespace system_utils {
Socket::Socket(void)
: is_enable_(false)
{

}

Socket::Socket(string ip, short port)
: is_enable_(false)
{
    this->create_socket(ip, port);
}

Socket::~Socket()
{
    this->close();
}

int 
Socket::close(void)
{
    if (is_enable_) {
        is_enable_ = false;
        int ret = ::close(socket_);
        if (ret < 0) {
            LOG_ERROR("close failed: %s", strerror(errno));
            return 1;
        }
    }

    return 0;
}

int 
Socket::shutdown(int how)
{
    if (is_enable_) {
        int ret = ::shutdown(socket_, how);
        if (ret < 0) {
            is_enable_ = false;
            LOG_ERROR("shutdown failed: %s", strerror(errno));
            return 1;
        }
    }

    return 0;
}

int 
Socket::get_socket(int &socket)
{
    if (is_enable_ == false) {
        LOG_WARN("Please create socket first.");
        return 1;
    }

    socket = socket_;
    return 0;
}

int 
Socket::get_ip_info(string &ip, short &port)
{
    if (is_enable_ == false) {
        LOG_WARN("Please create socket first.");
        return 1;
    }

    ip = ip_;
    port = port_;

    return 0;
}

int 
Socket::set_socket(int clisock, struct sockaddr_in *cliaddr, socklen_t *addrlen)
{
    if (is_enable_ == true) {
        LOG_WARN("set_socket failed, there's already opened a socket.");
        return 1;
    }

    socket_ = clisock;
    is_enable_ = true;
    if (cliaddr == nullptr || addrlen == nullptr) {
        return 0;
    }

    char buf[128] = {0};
    const char *ret = ::inet_ntop(AF_INET, &(cliaddr->sin_addr), buf, *addrlen);
    if (ret == nullptr) {
        LOG_ERROR("inet_ntop: %s", strerror(errno));
        return 1;
    }

    ip_ = ret;
    port_ = ::ntohs(cliaddr->sin_port);

    return 0;
}

int 
Socket::create_socket(string ip, short port)
{
    if (is_enable_ == true) {
        LOG_WARN("create_socket failed, there's already opened a socket.");
        return 1;
    }

    ip_ = ip;
    port_ = port;
    memset(&addr_, 0, sizeof(struct sockaddr_in));

    int ret = ::inet_pton(AF_INET, ip_.c_str(), &addr_.sin_addr);
    if (ret  == 0) {
        LOG_WARN("Incorrect format of IP address");
        return 1;
    } else if (ret < 0) {
        LOG_ERROR("inet_pton: %s", strerror(errno));
        return 1;
    }

    
    addr_.sin_family = AF_INET;
    addr_.sin_port = ::htons(port_);

    // 创建套接字
    socket_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0) {
        LOG_ERROR("socket: %s", strerror(errno));
        return 1;
    }

    is_enable_ = true;

    return 0;
}

// 这里listen()合并了 bind()和listen
int 
Socket::listen(void)
{
    if (is_enable_ == false) {
        LOG_WARN("Please create socket first.");
        return 1;
    }

    int ret = ::bind(socket_, (sockaddr*)&addr_, sizeof(addr_));
    if (ret < 0) {
        LOG_ERROR("bind: %s", strerror(errno));
        this->close();
        return 1;
    }

    ret = ::listen(socket_, 5);
    if (ret < 0) {
        LOG_ERROR("listen: %s", strerror(errno));
        this->close();
        return 1;
    }

    return 0;
}

int Socket::connect(void)
{
    if (is_enable_ == false) {
        LOG_WARN("Please create socket first.");
        return 1;
    }

    int ret = ::connect(socket_, (sockaddr*)&addr_, sizeof(addr_));
    if (ret < 0) {
        LOG_ERROR("connect: %s", strerror(errno));
        this->close();
        return 1;
    }

    return 0;
}

int Socket::accept(int &clisock, struct sockaddr *cliaddr, socklen_t *addrlen)
{
    if (is_enable_ == false) {
        LOG_WARN("Please create socket first.");
        return 1;
    }

    int ret = ::accept(socket_, cliaddr, addrlen);
    if (ret < 0) {
        LOG_ERROR("accept: %s", strerror(errno));
        this->close();
        return 1; 
    }
    clisock = ret;

    return 0;
}

int 
Socket::recv(ByteBuffer &buff, int buff_size, int flags)
{
    if (is_enable_ == false) {
        LOG_WARN("Please create socket first.");
        return 0;
    }

    size_t remain_size = buff_size;
    do {
        ssize_t ret = ::recv(socket_, buff.get_write_buffer_ptr(), buff.get_cont_write_size(), flags);
        if (ret < 0) {
            LOG_ERROR("recv: %s", strerror(errno));
            return 0;
        }
        
        remain_size -= ret;

        if (ret == 0 || buff.idle_size() == 0) {
            break;
        }
    }  while (remain_size > 0);
   
    return buff.data_size();
}

int 
Socket::send(ByteBuffer &buff, int buff_size, int flags)
{
    if (is_enable_ == false) {
        LOG_WARN("Please create socket first.");
        return 0;
    }

    size_t remain_size = buff_size;
    do {
        int ret = ::send(socket_, buff.get_read_buffer_ptr(), buff.get_cont_read_size(), flags);
        if (ret == -1) {
            LOG_ERROR("send: %s", strerror(errno));
            return 0;
        }

        remain_size -= ret;
        if (ret == 0 || buff.idle_size() == 0) {
            break;
        }
    }  while (remain_size > 0);
   
    return buff_size - remain_size;
}

}