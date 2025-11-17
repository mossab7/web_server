#include "FdManager.hpp"

FdManager::FdManager(Epoll &epoll) : _epoll(epoll) {}
FdManager::~FdManager()
{
    Logger logger;
    logger.debug("FdManager destructor called");
    for (std::map<int, EventHandler *>::iterator it = fd_map.begin(); it != fd_map.end(); ++it)
    {
        logger.debug("Cleaning up fd: " + intToString(it->first));
        it->second->destroy();
    }
    fd_map.clear();
}
void FdManager::add(int fd, EventHandler *handler, int events, bool timeout)
{
    _epoll.add_fd(fd, events);
    fd_map[fd] = handler;
    if (timeout)
        timeout_map[fd] = handler;
}
void FdManager::remove(int fd)
{
    Logger logger;
    logger.debug("FdManager removing fd: " + intToString(fd));
    std::map<int, EventHandler *>::iterator it = fd_map.find(fd);
    if (it != fd_map.end())
    {
        if (timeout_map.find(fd) != timeout_map.end())
            timeout_map.erase(fd);
        _epoll.remove_fd(fd);
        it->second->destroy();
        fd_map.erase(it);
    }
}
void FdManager::detachFd(int fd)
{
    Logger logger;
    std::map<int, EventHandler *>::iterator it = fd_map.find(fd);
    if (it != fd_map.end())
    {
        logger.debug("FdManager detaching fd: " + intToString(fd));
        if (timeout_map.find(fd) != timeout_map.end())
            timeout_map.erase(fd);
        _epoll.remove_fd(fd);
        fd_map.erase(it);
    }
}
EventHandler *FdManager::getOwner(int fd)
{
    std::map<int, EventHandler *>::iterator it = fd_map.find(fd);
    if (it != fd_map.end())
    {
        return it->second;
    }
    return NULL;
}
bool FdManager::exists(int fd)
{
    return fd_map.find(fd) != fd_map.end();
}
void FdManager::modify(int fd, uint32_t events)
{
    if (exists(fd))
    {
        _epoll.modify_fd(fd, events);
    }
}
void FdManager::modify(EventHandler *handler, uint32_t events)
{
    modify(handler->get_fd(), events);
}
std::map<int, EventHandler *> &FdManager::getEventHandlersTimeouts()
{
    return timeout_map;
}