#include "../../include/server/Socket.hpp"
#include "../../include/server/Epoll.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include "Logger.hpp"
Socket::Socket(const Socket &other) : _fd(other._fd), _event(other._event), _epoll(other._epoll) {}

void Socket::register_epoll(Epoll *epoll)
{
    _epoll = epoll;
}

void Socket::create_socket(int domain, int type, int protocol)
{
    _epoll = NULL;
    _fd = ::socket(domain, type, protocol);
    if (_fd == -1)
    {
        throw std::runtime_error("Failed to create socket");
    }
}

Socket::Socket()
{
    create_socket(AF_INET, SOCK_STREAM, 0);
}

Socket::Socket(int fd)
{
    _fd = fd;
}

std::string intToString(int value);

Socket::~Socket()
{
    if (_fd != -1)
    {
        ::close(_fd);
    }
}

void Socket::bind()
{
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    int opt = 1;
    if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        throw std::runtime_error("Failed to set socket options");
    }

    bind(address);
}

void Socket::bind(struct sockaddr_in address)
{
    if (::bind(_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        throw std::runtime_error("Failed to bind socket");
    }
}

void Socket::listen()
{
    if (::listen(_fd, SOMAXCONN) < 0)
    {
        throw std::runtime_error("Failed to listen on socket");
    }
}

int Socket::accept()
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = ::accept(_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0)
    {
        throw std::runtime_error("Failed to accept connection");
    }

    return client_fd;
}

void Socket::connect(std::string ip, int port)
{
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &address.sin_addr) <= 0)
    {
        throw std::runtime_error("Invalid address");
    }

    connect(address);
}

void Socket::connect(struct sockaddr_in address)
{
    if (::connect(_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        throw std::runtime_error("Failed to connect");
    }
}

void Socket::set_non_blocking()
{
    int flags = fcntl(_fd, F_GETFL, 0);
    if (flags == -1)
    {
        throw std::runtime_error("Failed to get socket flags");
    }

    if (fcntl(_fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        throw std::runtime_error("Failed to set socket non-blocking");
    }
}

void Socket::connect(std::string ip, int port, sa_family_t family)
{
    struct sockaddr_in address;
    address.sin_family = family;
    address.sin_port = htons(port);

    if (inet_pton(family, ip.c_str(), &address.sin_addr) <= 0)
    {
        throw std::runtime_error("Invalid address");
    }

    connect(address);
}

ssize_t Socket::send(const char *buffer, size_t length, int flags)
{
    ssize_t result = ::send(_fd, buffer, length, flags);
    if (result == -1)
    {
        throw std::runtime_error("Failed to send data");
    }
    return result;
}

ssize_t Socket::recv(char *buffer, size_t length, int flags)
{
    ssize_t result = ::recv(_fd, buffer, length, flags);
    if (result == -1)
    {
        throw std::runtime_error("Failed to receive data");
    }
    return result;
}

void Socket::close()
{
    if (_fd != -1)
    {
        ::close(_fd);
        _fd = -1;
    }
}

int Socket::get_fd() const
{
    return _fd;
}

uint32_t Socket::get_event() const
{
    return _event;
}

uint32_t operator|(uint32_t event, const Socket &socket)
{
    return event | socket.get_event();
}

uint32_t operator&(uint32_t event, const Socket &socket)
{
    return event & socket.get_event();
}

uint32_t operator|(const Socket &socket, uint32_t event)
{
    return socket.get_event() | event;
}

uint32_t operator&(const Socket &socket, uint32_t event)
{
    return socket.get_event() & event;
}

bool operator==(const Socket &lhs, const Socket &rhs)
{
    return lhs.get_fd() == rhs.get_fd();
}
