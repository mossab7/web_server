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
        Socket _socket;
    public:
        EventHandler(ServerConfig &config ,FdManager &fdm);
        EventHandler(ServerConfig &config ,FdManager &fdm, Socket &socket);
        virtual ~EventHandler() {}
        virtual int get_fd() const = 0;
        virtual void onReadable() {};
        virtual void onWritable() {};
        virtual void onError() {};
};

inline EventHandler::EventHandler(ServerConfig &config, FdManager &fdm, Socket &socket) : _fd_manager(fdm), _config(config), _socket(socket) {}
inline EventHandler::EventHandler(ServerConfig &config, FdManager &fdm) : _fd_manager(fdm), _config(config), _socket() {}

#endif //EVENT_HANDLER_HPP