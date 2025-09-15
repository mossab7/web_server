#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Socket.hpp"
#include <sstream>

enum parse_state 
{
    STATE_REQUEST_LINE,
    STATE_HEADERS,
    STATE_BODY,
    STATE_DONE,
    STATE_READY_TO_SEND,
    STATE_ERROR
};

class Client
{
private:
    Socket _socket;
    parse_state _state;
    std::string _buffer;
    std::string _response_buffer;
    size_t _response_pos;
    size_t _content_length;
    std::string _method;
    std::string _url;
    std::string _http_version;
    std::string _headers;
    std::string _body;
    Client(const Client &other);
    Client &operator=(const Client &other);
    void parse_request();
    void generate_echo_response();
public:
    Client();
    Client(const Socket &socket);
    void read();
    void send();
    bool needs_write() const;
    bool is_done() const;
    ~Client();
};

Client::Client() : _state(STATE_REQUEST_LINE), _response_pos(0), _content_length(0) {}

Client::Client(const Socket &socket) : _socket(socket), _state(STATE_REQUEST_LINE), _response_pos(0), _content_length(0) {}

Client::~Client() {}

void Client::read()
{
    char buffer[4096];
    ssize_t bytes_received = _socket.recv(buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0)
    {
        buffer[bytes_received] = '\0';
        _buffer.append(buffer);
        parse_request();
    }
    else if (bytes_received == 0)
    {
        _state = STATE_DONE;
    }
    else
    {
        throw std::runtime_error("Failed to read from socket");
    }
}

void Client::parse_request()
{
    size_t pos = 0;
    
    if (_state == STATE_REQUEST_LINE)
    {
        size_t end_line = _buffer.find("\r\n", pos);
        if (end_line != std::string::npos)
        {
            std::string request_line = _buffer.substr(pos, end_line - pos);
            std::istringstream iss(request_line);
            iss >> _method >> _url >> _http_version;
            pos = end_line + 2;
            _state = STATE_HEADERS;
        }
    }
    
    if (_state == STATE_HEADERS)
    {
        size_t header_end = _buffer.find("\r\n\r\n", pos);
        if (header_end != std::string::npos)
        {
            _headers = _buffer.substr(pos, header_end - pos);
            pos = header_end + 4;
            
            size_t cl_pos = _headers.find("Content-Length:");
            if (cl_pos != std::string::npos)
            {
                size_t cl_start = _headers.find(":", cl_pos) + 1;
                size_t cl_end = _headers.find("\r\n", cl_start);
                std::string cl_str = _headers.substr(cl_start, cl_end - cl_start);
                _content_length = std::stoul(cl_str);
                _state = STATE_BODY;
            }
            else
            {
                _content_length = 0;
                _state = STATE_READY_TO_SEND;
                generate_echo_response();
            }
        }
    }
    
    if (_state == STATE_BODY)
    {
        if (_buffer.length() - pos >= _content_length)
        {
            _body = _buffer.substr(pos, _content_length);
            _state = STATE_READY_TO_SEND;
            generate_echo_response();
        }
    }
}

void Client::generate_echo_response()
{
    std::ostringstream response;
    
    // Create echo content
    std::string echo_content = "Echo Response:\n";
    echo_content += "Method: " + _method + "\n";
    echo_content += "URL: " + _url + "\n";
    echo_content += "HTTP Version: " + _http_version + "\n";
    echo_content += "Headers:\n" + _headers + "\n";
    if (!_body.empty())
    {
        echo_content += "Body:\n" + _body + "\n";
    }
    
    // Build HTTP response
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: text/plain\r\n";
    response << "Content-Length: " << echo_content.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << echo_content;
    
    _response_buffer = response.str();
    _response_pos = 0;
}

void Client::send()
{
    if (_state == STATE_READY_TO_SEND && !_response_buffer.empty())
    {
        size_t remaining = _response_buffer.length() - _response_pos;
        ssize_t bytes_sent = _socket.send(_response_buffer.c_str() + _response_pos, remaining, 0);
        
        if (bytes_sent > 0)
        {
            _response_pos += bytes_sent;
            if (_response_pos >= _response_buffer.length())
            {
                _state = STATE_DONE;
            }
        }
        else
        {
            throw std::runtime_error("Failed to send response");
        }
    }
}

bool Client::needs_write() const
{
    return _state == STATE_READY_TO_SEND && _response_pos < _response_buffer.length();
}

bool Client::is_done() const
{
    return _state == STATE_DONE;
}

#endif // CLIENT_HPP