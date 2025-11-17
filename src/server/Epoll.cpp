#include "../../include/server/Epoll.hpp"
#include <sys/epoll.h>
#include <unistd.h>
#include <stdexcept>
#include <signal.h>

extern volatile sig_atomic_t g_shutdown;

Epoll::Epoll()
{
    _epoll_fd = epoll_create1(0);
    if (_epoll_fd == -1)
    {
        throw std::runtime_error("Failed to create epoll file descriptor");
    }
}

Epoll::~Epoll()
{
    if (_epoll_fd != -1)
    {
        close(_epoll_fd);
    }
}

void Epoll::add_fd(Socket &socket, uint32_t events)
{
    add_fd(socket.get_fd(), events);
    socket.register_epoll(this);
}

void Epoll::add_fd(int fd, uint32_t events)
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
    if (::epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1)
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

std::vector<epoll_event> Epoll::wait(int timeout)
{
    std::vector<epoll_event> ready_events;
    struct epoll_event events[MAX_EVENTS];
    int num_events = ::epoll_wait(_epoll_fd, events, MAX_EVENTS, timeout);
    if (num_events == -1)
    {
        if (g_shutdown)
        {
            return ready_events;
        }
        throw std::runtime_error("Failed to wait for epoll events");
    }

    for (int i = 0; i < num_events; i++)
    {
        ready_events.push_back(events[i]);
    }

    return ready_events;
}

void Epoll::remove_fd(Socket &socket)
{
    remove_fd(socket.get_fd());
}

int Epoll::getFd()
{
    return _epoll_fd;
}