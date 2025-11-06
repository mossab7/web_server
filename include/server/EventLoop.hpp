#ifndef EVENT_LOOP_HPP
#define EVENT_LOOP_HPP

#include "Epoll.hpp"
#include "FdManager.hpp"
#include "Logger.hpp"

class EventLoop
{
    private:
        Epoll epoll;
        Logger logger;
    public:
        FdManager fd_manager;
        EventLoop();
        ~EventLoop();
        void run();
        void expireTimeouts();
        int computeNextTimeout();
};

#endif //EVENT_LOOP_HPP