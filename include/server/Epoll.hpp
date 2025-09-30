#ifndef EPOLL_HPP
#define EPOLL_HPP

#include <sys/epoll.h>
#include <unistd.h>
#include <stdexcept>
#include <vector>
#include "Socket.hpp"

#define EVENT_HAS_READ(event) ((event) & EPOLLIN)
#define EVENT_HAS_WRITE(event) ((event) & EPOLLOUT)
#define EVENT_HAS_ERROR(event) ((event) & (EPOLLERR | EPOLLHUP))
#define EVENT_HAS_EDGE_TRIGGERED(event) ((event) & EPOLLET)
#define EVENT_HAS_ONE_SHOT(event) ((event) & EPOLLONESHOT)
#define EVENT_HAS_PRIORITY(event) ((event) & EPOLLPRI)
#define EVENT_READ_WRITE (EPOLLIN | EPOLLOUT)
#define EVENT_READ_WRITE_ERROR (EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP)

#define MAX_EVENTS 1024

class Epoll
{
private:
    int _epoll_fd;
    Epoll(const Epoll &other);
    Epoll &operator=(const Epoll &other);
    
public:
    Epoll();
    ~Epoll();
    void add_fd(Socket &socket, uint32_t events = EPOLLIN);
    void add_fd(int fd, uint32_t events = EPOLLIN);
    void modify_fd(int fd, uint32_t events);
    void remove_fd(int fd);
    void remove_fd(Socket &socket);
    void modify_fd(Socket &socket, uint32_t events);
    std::vector<Socket> wait(int timeout = -1);
    static Epoll& getInstance();
};


#endif //EPOLL_HPP