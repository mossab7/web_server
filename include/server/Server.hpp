#ifndef SERVER_HPP
#define SERVER_HPP

#include "EventHandler.hpp"
#include "Socket.hpp"
#include "../Config/ConfigParser.hpp"

class Server : public EventHandler
{
    private:
    public:
        Server(ServerConfig &config, FdManager &fdm);
        ~Server();
        int get_fd() const;
        void onReadable();
        void onWritable();
        void onError();
};

#endif //SERVER_HPP