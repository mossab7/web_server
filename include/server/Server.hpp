#ifndef SERVER_HPP
#define SERVER_HPP

#include "EventHandler.hpp"
#include "Socket.hpp"
#include "../Config/ConfigParser.hpp"

class Server : public EventHandler
{
    private:
        Socket _socket;
    public:
        Server(ServerConfig &config, FdManager &fdm);
        ~Server();
        int get_fd() const;
        void destroy();
        void onEvent(uint32_t events);
        void onReadable();
        void onWritable();
        void onError();
        void onTimeout();
        int get_fd();
};

#endif //SERVER_HPP