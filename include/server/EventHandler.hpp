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
        time_t      _expiresAt;
        virtual void _updateExpiresAt(time_t new_expires) { _expiresAt = new_expires; };
    public:
        EventHandler(ServerConfig &config ,FdManager &fdm, time_t expires_at);
        virtual ~EventHandler() {}
        virtual void onEvent(uint32_t events) = 0;
        virtual void destroy() {// evey handler implement it's own destroy
            //delete this;
        };
        virtual int get_fd() = 0;
        virtual void onReadable() {};
        virtual void onWritable() {};
        virtual void onError() {};
        virtual void onTimeout() {};
        virtual time_t getExpiresAt() const { return _expiresAt; };
};

inline EventHandler::EventHandler(ServerConfig &config, FdManager &fdm, time_t expires_at) : _fd_manager(fdm), _config(config), _expiresAt(expires_at) {}

#endif //EVENT_HANDLER_HPP
