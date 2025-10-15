#ifndef EVENT_HANDLER_HPP
#define EVENT_HANDLER_HPP

#include "../Config/ConfigParser.hpp"
#include "Socket.hpp"

class FdManager;  // Forward declaration

class EventHandler
{
    protected:
        FdManager &_fd_manager;
        ServerConfig _config;
    public:
        EventHandler(ServerConfig &config ,FdManager &fdm);
        virtual ~EventHandler() {}
        virtual int get_fd() const = 0;
        virtual void onEvent(uint32_t events) = 0;
        virtual void onReadable() {};
        virtual void onWritable() {};
        virtual void onError() {};
};

inline EventHandler::EventHandler(ServerConfig &config, FdManager &fdm) : _fd_manager(fdm), _config(config) {}

#endif //EVENT_HANDLER_HPP