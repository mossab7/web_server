#ifndef EPOLL_HPP
#define EPOLL_HPP

#include <sys/epoll.h>
#include <unistd.h>
#include <stdexcept>
#include <vector>
#include "Socket.hpp"

#define IS_READ_EVENT(event) ((event) & EPOLLIN)
#define IS_WRITE_EVENT(event) ((event) & EPOLLOUT)
#define IS_ERROR_EVENT(event) ((event) & (EPOLLERR | EPOLLHUP))
#define EDGE_TRIGGERED_EVENT(event) ((event) & EPOLLET)
#define ONE_SHOT_EVENT(event) ((event) & EPOLLONESHOT)
#define PRIORITY_EVENT(event) ((event) & EPOLLPRI)
#define READ_WRITE_EVENT (EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP)
#define READ_EVENT (EPOLLIN | EPOLLERR | EPOLLHUP)
#define WRITE_EVENT (EPOLLOUT | EPOLLERR | EPOLLHUP)

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
    std::vector<epoll_event> wait(int timeout = -1);
    int getFd();
};


#endif //EPOLL_HPP