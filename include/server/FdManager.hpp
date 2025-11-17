#ifndef FD_MANAGER_HPP
#define FD_MANAGER_HPP

#include <map>
#include "EventHandler.hpp"
#include "Epoll.hpp"
#include "../utils/Logger.hpp"
std::string intToString(int value);
class FdManager
{
private:
    Epoll &_epoll;
    std::map<int, EventHandler *> fd_map;
    std::map<int, EventHandler *> timeout_map;

public:
    FdManager(Epoll &epoll);
    ~FdManager();
    void add(int fd, EventHandler *handler, int events, bool timeout = true);
    void remove(int fd);
    void detachFd(int fd);

    EventHandler *getOwner(int fd);
    bool exists(int fd);
    void modify(int fd, uint32_t events);
    void modify(EventHandler *handler, uint32_t events);
    std::map<int, EventHandler *> &getEventHandlersTimeouts();
};

#endif // FD_MANAGER_HPP