#ifndef WEBSERV_CLIENT_HPP
#define WEBSERV_CLIENT_HPP

#define DEFAULT_CLIENT_TIMEOUT 7

#include "EventHandler.hpp"
#include "FdManager.hpp"
#include "Socket.hpp"
#include "HTTPParser.hpp"
#include "Response.hpp"
#include "ConfigParser.hpp"
#include "Logger.hpp"
#include "RequestHandler.hpp"
#include <time.h>

enum ClientState
{
    ST_READING,
    ST_PROCESSING,
    ST_PARSEERROR,
    ST_SENDING,
    ST_SENDCOMPLETE,
    ST_CLOSED,
    ST_ERROR
};

class Client : public EventHandler
{
    Socket _socket;
    Logger logger;

    HTTPParser _req;
    HTTPResponse _resp;

    RequestHandler _handler;

    char _readBuff[BUFF_SIZE];
    char _sendBuff[BUFF_SIZE];

    std::string _strFD;
    ClientState _state;

    bool _keepAlive;

    bool _shouldKeepAlive();

    void _closeConnection();

    void _processError();
    void _processRequest();

    bool _readData();
    bool _sendData();

public:
    Client(int socket_fd, ServerConfig &config, FdManager &fdm);
    ~Client();

    void reset();

    void setCGIMode(bool b);

    int get_fd() const;
    void destroy();
    void onEvent(uint32_t events);
    void onReadable();
    void onWritable();
    void onError();
    void onTimeout();
    int get_fd();
};

#endif // CLIENT_HPP
