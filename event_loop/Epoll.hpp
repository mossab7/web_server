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
    void Epoll::modify_fd(Socket &socket, uint32_t events);
    std::vector<Socket> wait(int timeout = -1);
};

Epoll::Epoll()
{
    _epoll_fd = ::epoll_create(0);
    if (_epoll_fd == -1) 
    {
        throw std::runtime_error("Failed to create epoll instance");
    }
}

Epoll::~Epoll()
{
    if (_epoll_fd != -1) 
    {
        ::close(_epoll_fd);
    }
}

void Epoll::add_fd(Socket &socket, uint32_t events = EPOLLIN)
{
    add_fd(socket.get_fd(), events);
    socket.register_epoll(this);
}

void Epoll::add_fd(int fd, uint32_t events = EPOLLIN)
{
    struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    if (::epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) 
    {
        throw std::runtime_error("Failed to add file descriptor to epoll");
    }
}

void Epoll::remove_fd(int fd)
{
    if (::epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, nullptr) == -1) 
    {
        throw std::runtime_error("Failed to remove file descriptor from epoll");
    }
}

void Epoll::modify_fd(int fd, uint32_t events)
{
    struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    if (::epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &event) == -1) 
    {
        throw std::runtime_error("Failed to modify file descriptor in epoll");
    }
}

void Epoll::modify_fd(Socket &socket, uint32_t events)
{
    modify_fd(socket.get_fd(), events);
}

std::vector<Socket> Epoll::wait(int timeout = -1)
{
    std::vector<Socket> ready_fds;
    struct epoll_event events[MAX_EVENTS];
    int num_events = ::epoll_wait(_epoll_fd, events, MAX_EVENTS, timeout);
    if (num_events == -1)
    {
        throw std::runtime_error("Failed to wait for epoll events");
    }
    for (int i = 0; i < num_events; ++i) 
    {
        ready_fds.push_back(Socket(events[i].data.fd, events[i].events));
    }
    return ready_fds;
}

void Epoll::remove_fd(Socket &socket)
{
    remove_fd(socket.get_fd());
}

#endif // EPOLL_HPP