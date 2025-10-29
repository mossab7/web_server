#ifndef FD_MANAGER_HPP
#define  FD_MANAGER_HPP

#include <map>
#include "EventHandler.hpp"
#include "Epoll.hpp"
#include "../utils/Logger.hpp"
std::string intToString(int value);
class FdManager {
    private:
    Epoll &_epoll;
    std::map<int, EventHandler*> fd_map; 
public:
    FdManager(Epoll &epoll) : _epoll(epoll) {}
    ~FdManager() 
    {
        Logger logger;
        logger.debug("FdManager destructor called");
        for (std::map<int, EventHandler*>::iterator it = fd_map.begin(); it != fd_map.end(); ++it) {
            logger.debug("Cleaning up fd: " + intToString(it->first));
            it->second->destroy();
        }
        fd_map.clear();
    }
    void add(int fd, EventHandler* handler, int events)
    {
        _epoll.add_fd(fd, events);
        fd_map[fd] = handler;
    }
    void remove(int fd) 
    {
        std::map<int, EventHandler*>::iterator it = fd_map.find(fd);
        if (it != fd_map.end()) 
        {
            _epoll.remove_fd(fd);
            it->second->destroy();
            fd_map.erase(it);
        }
    }
    void detachFd(int fd)
    {
        std::map<int, EventHandler*>::iterator it = fd_map.find(fd);
        if (it != fd_map.end()) 
        {
            _epoll.remove_fd(fd);
            fd_map.erase(it);
        }
    }
    EventHandler* getOwner(int fd) 
    {
        std::map<int, EventHandler*>::iterator it = fd_map.find(fd);
        if (it != fd_map.end()) {
            return it->second;
        }
        return NULL;
    }
    bool exists(int fd) 
    {
        return fd_map.find(fd) != fd_map.end();
    }
    void modify(int fd, uint32_t events) 
    {
        if (exists(fd)) {
            _epoll.modify_fd(fd, events);
        }
    }
    void modify(EventHandler* handler, uint32_t events) 
    {
        modify(handler->get_fd(), events);
    }

};

#endif //FD_MANAGER_HPP