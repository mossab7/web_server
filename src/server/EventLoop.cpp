#include "EventLoop.hpp"
#include <sstream>
#include <csignal>

// Helper macro for converting to string
#define SSTR(x) static_cast<std::ostringstream &>((std::ostringstream() << x)).str()

// External shutdown flag
extern volatile sig_atomic_t g_shutdown;

EventLoop::EventLoop() : epoll(), fd_manager(epoll)
{
    
}

void EventLoop::run()
{
    logger.info("Event loop started");
    while(!g_shutdown)
    {
        std::vector<epoll_event> events = epoll.wait(1000); // 1 second timeout for signal handling
        for (size_t i = 0; i < events.size(); i++) 
        {
            try 
            {
                EventHandler* handler = fd_manager.getOwner(events[i].data.fd);
                if (handler == NULL) {
                    logger.warning("Event for unknown fd: " + SSTR(events[i].data.fd));
                    continue;
                }
                handler->onEvent(events[i].events);
            }
            catch (const std::exception &e) 
            {
                logger.error("Exception in event loop: " + std::string(e.what()));
                // Try to cleanup the problematic handler
                try {
                        EventHandler* handler = fd_manager.getOwner(events[i].data.fd);
                        if (handler) {
                            fd_manager.remove(events[i].data.fd);
                            delete handler;
                    }
                } 
                catch (...) 
                {
                    logger.error("Failed to cleanup handler after exception");
                }
            }
        }
    }
}

EventLoop::~EventLoop()
{
    logger.info("Event loop terminated");
}
