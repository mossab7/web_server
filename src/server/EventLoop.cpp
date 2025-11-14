#include "EventLoop.hpp"
#include <sstream>
#include <csignal>
#include <limits>

// Helper macro for converting to string
#define SSTR(x) static_cast<std::ostringstream &>((std::ostringstream() << x)).str()

// External shutdown flag
extern volatile sig_atomic_t g_shutdown;

EventLoop::EventLoop() : epoll(), fd_manager(epoll)
{
    
}

int EventLoop::computeNextTimeout()
{
    std::map<int, EventHandler*>& handlers = fd_manager.getEventHandlersTimeouts();
    if (handlers.empty())
        return -1;

    time_t now = time(NULL);
    time_t min_expires = std::numeric_limits<time_t>::max();

    for (std::map<int, EventHandler*>::iterator it = handlers.begin(); it != handlers.end(); ++it) 
    {
        EventHandler* handler = it->second;
        if (!handler)
            continue;
        time_t ex = handler->getExpiresAt();
        if (ex < min_expires)
            min_expires = ex;
    }

    if (min_expires <= now)
        return 0;

    long seconds = static_cast<long>(min_expires - now);
    long ms = seconds * 1000L;
    if (ms > std::numeric_limits<int>::max())
        return std::numeric_limits<int>::max();
    return static_cast<int>(ms);
}

void EventLoop::expireTimeouts()
{
    std::map<int, EventHandler*> handlers = fd_manager.getEventHandlersTimeouts();
    
    for (std::map<int, EventHandler*>::iterator it = handlers.begin(); it != handlers.end(); ++it)
    {
        time_t now = time(NULL);
       // logger.info("Checking timeout for fd: " + SSTR(it->first));
       // logger.info("time now : " + SSTR(now) + " will expires in: " + SSTR(it->second->getExpiresAt()) + " seconds");
        EventHandler* handler = it->second;
        if (!handler)
            continue;
        if (handler->getExpiresAt() <= now)
        {
            //it++;
            try {
                handler->onEvent(TIMEOUT_EVENT);
            } catch (const std::exception &e) {
                logger.error(std::string("Exception in onEvent(): ") + e.what());
            } catch (...) {
                logger.error("Unknown exception in onEvent()");
            }
        }
        //logger.info("Finished checking timeout for fd");
    }
}

void EventLoop::run()
{
    logger.info("Event loop started");
    while(!g_shutdown)
    {
        //logger.debug("next epoll wait with timeout: " + SSTR(computeNextTimeout()) + " ms");
        std::vector<epoll_event> events = epoll.wait(15000); // 15 seconds max wait
        // if (events.empty())
        // {
        //     logger.debug("No events, expired timeouts processed");
        //     continue;
        // }
       // logger.debug("iterate");
        expireTimeouts();
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
                try {
                        EventHandler* handler = fd_manager.getOwner(events[i].data.fd);
                        if (handler)
                            fd_manager.remove(events[i].data.fd);
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
