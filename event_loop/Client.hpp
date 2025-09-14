#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Socket.hpp"

enum parse_state 
{
    STATE_REQUEST_LINE,
    STATE_HEADERS,
    STATE_BODY,
    STATE_DONE,
    STATE_ERROR
};

class Client
{
private:
    Socket _socket;
    parse_state _state;
    std::string _buffer;
    size_t _buffer_pos;
    size_t _content_length;
    std::string _method;
    std::string _url;
    std::string _http_version;
    std::string _headers;
    std::string _body;
    Client(const Client &other);
    Client &operator=(const Client &other);
public:
    Client();
    Client(const Socket &socket);
    void read();
    void send();
    ~Client();
};

#endif // CLIENT_HPP