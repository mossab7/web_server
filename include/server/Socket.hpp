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
    Socket(int fd, uint32_t event) : _fd(fd), _event(event), _epoll(NULL) {};
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

// Operator overloads
uint32_t operator|(uint32_t event, const Socket &socket);
uint32_t operator&(uint32_t event, const Socket &socket);
uint32_t operator|(const Socket &socket, uint32_t event);
uint32_t operator&(const Socket &socket, uint32_t event);
bool operator==(const Socket &lhs, const Socket &rhs);

#define EVENT_HAS_ERROR(event) ((event) & (EPOLLERR | EPOLLHUP))
#define EVENT_HAS_READ(event) ((event) & EPOLLIN)
#define EVENT_HAS_WRITE(event) ((event) & EPOLLOUT)

#endif //SOCKET_HPP