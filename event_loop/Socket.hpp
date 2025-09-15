#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <unistd.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdexcept>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <fcntl.h>

class Epoll; 

struct fromFdTag {};

class Socket
{
private:
    int _fd;
    uint32_t _event;
    Epoll *_epoll;
    Socket &operator=(const Socket &other);
    void create_socket(int domain, int type, int protocol);
    public:
    Socket();
    Socket(const Socket &other);
    Socket(fromFdTag, int fd);
    Socket(int fd, uint32_t event) : _fd(fd), _event(event), _epoll(nullptr) {};
    Socket(int _socket_domain, int _socket_type = SOCK_STREAM, int _protocol = 0);
    ~Socket();
    void bind();
    void bind(struct sockaddr_in address);
    void listen();
    void set_non_blocking();
    Socket accept();
    void connect(std::string ip, int port);
    void connect(struct sockaddr_in address);
    void connect(std::string ip, int port, sa_family_t family);
    ssize_t send(const char *buffer, size_t length, int flags);
    ssize_t recv(char *buffer, size_t length, int flags);
    int get_fd() const;
    uint32_t get_event() const;
    void register_epoll(Epoll *epoll);
    void close();
};

Socket::Socket(const Socket &other) : _fd(other._fd), _event(other._event), _epoll(other._epoll) {}

void Socket::register_epoll(Epoll *epoll)
{
    _epoll = epoll;
}

void Socket::create_socket(int domain, int type, int protocol)
{
    _epoll = nullptr;
    _fd = ::socket(domain, type, protocol);
    if (_fd == -1) {
        throw std::runtime_error("Failed to create socket");
    }
}

Socket::Socket()
{
    create_socket(AF_INET, SOCK_STREAM, 0);
}

Socket::Socket(fromFdTag, int fd)
{
    _fd = fd;
}

Socket::Socket(int _socket_domain, int _socket_type = SOCK_STREAM, int _protocol = 0)
{
    create_socket(_socket_domain, _socket_type, _protocol);
}

Socket::~Socket() 
{
    if (_fd != -1) {
        ::close(_fd);
    }
}

void Socket::bind() 
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (::bind(_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        throw std::runtime_error("Failed to bind socket");
    }
}

void Socket::bind(struct sockaddr_in address)
{
    if (::bind(_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        throw std::runtime_error("Failed to bind socket");
    }
}

void Socket::listen()
{
    if (::listen(_fd, SOMAXCONN) == -1) {
        throw std::runtime_error("Failed to listen on socket");
    }
}

Socket Socket::accept()
{
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t client_len = sizeof(client_addr);
    int client_fd = ::accept(_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd == -1) {
        throw std::runtime_error("Failed to accept connection");
    }
    return Socket(fromFdTag(), client_fd);
}

void Socket::connect(std::string ip, int port)
{
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (::connect(_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        throw std::runtime_error("Failed to connect socket");
    }
}

void Socket::connect(struct sockaddr_in address)
{
    if (::connect(_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        throw std::runtime_error("Failed to connect socket");
    }
}

void Socket::set_non_blocking()
{
    int flags = fcntl(_fd, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error("Failed to get socket flags");
    }
    if (fcntl(_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error("Failed to set socket to non-blocking");
    }
}

void Socket::connect(std::string ip, int port, sa_family_t family)
{
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = family;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (::connect(_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        throw std::runtime_error("Failed to connect socket");
    }
}

ssize_t Socket::send(const char *buffer, size_t length, int flags)
{
    ssize_t bytes_sent = ::send(_fd, buffer, length, flags);
    if (bytes_sent == -1) {
        throw std::runtime_error("Failed to send data");
    }
    return bytes_sent;
}

ssize_t Socket::recv(char *buffer, size_t length, int flags)
{
    ssize_t bytes_received = ::recv(_fd, buffer, length, flags);
    if (bytes_received == -1) {
        throw std::runtime_error("Failed to receive data");
    }
    return bytes_received;
}

void Socket::close()
{
    if (_fd != -1) 
    {
        if (_epoll)
            _epoll->remove_fd(*this);
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

int operator|(uint32_t ev, const Socket &rhs)
{
    return (ev | rhs.get_event());
}

int operator&(uint32_t ev, const Socket &rhs)
{
    return (ev & rhs.get_event());
}

int operator|(const Socket &rhs, uint32_t ev)
{
    return (rhs.get_event() | ev);
}

int operator&(const Socket &rhs, uint32_t ev)
{
    return (rhs.get_event() & ev);
}

int operator==(const Socket &lhs, const Socket &rhs)
{
    return (lhs.get_fd() == rhs.get_fd());
}

#endif //SOCKET_HPP