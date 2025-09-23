#include "../../include/server/Client.hpp"
#include "../../include/utils/Logger.hpp"
#include <cstring>
#include <iostream>
#include <algorithm>
#include <sstream>

// Helper function for converting int to string in C++98
std::string intToString(int value) 
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

Client::Client() : _socket(), _response(NULL), _state(READING_REQUEST) 
{
    memset(_readBuffer, 0, BUFF_SIZE);
}

Client::Client(const Socket &socket) : _socket(socket), _response(NULL), _state(READING_REQUEST) 
{
    memset(_readBuffer, 0, BUFF_SIZE);
}

Client::~Client() 
{
    if (_response) {
        delete _response;
        _response = NULL;
    }
}

bool Client::readRequest()
{
    if (_state != READING_REQUEST) {
        return false;
    }
    
    // Read data from socket
    ssize_t bytesRead = _socket.recv(_readBuffer, BUFF_SIZE - 1, 0);
    
    if (bytesRead < 0) {
        // Error reading
        _state = ERROR_STATE;
        return false;
    }
    
    if (bytesRead == 0) {
        // Client closed connection
        _state = CLOSED;
        return false;
    }
    
    // Null terminate for safety
    _readBuffer[bytesRead] = '\0';
    
    // Feed chunk to parser
    _parser.addChunk(_readBuffer, bytesRead);
    
    // Check parser state
    if (_parser.isError()) {
        _state = ERROR_STATE;
        _buildErrorResponse(400, "Bad Request");
        return false;
    }
    
    if (_parser.isComplete()) {
        _state = PROCESSING;
        _processRequest();
        return false; // No more reading needed
    }
    
    // More data expected
    return true;
}

bool Client::sendResponse()
{
    if (_state != SENDING_RESPONSE || !_response) {
        return false;
    }
    
    return _sendResponseChunk();
    
}

void Client::_processRequest()
{
    // Create response based on parsed request
    _response = new HTTPResponse(200, "OK", "HTTP/1.1");
    
    // Add basic headers
    _response->addHeader("Server", "WebServ/1.0");
    _response->addHeader("Connection", shouldKeepAlive() ? "keep-alive" : "close");
    
    // For now, just echo back request info as a simple response
    std::string body = "<html><body>";
    body += "<h1>Request Received</h1>";
    body += "<p>Method: " + _parser.getMethod() + "</p>";
    body += "<p>URI: " + _parser.getUri() + "</p>";
    body += "<p>Version: " + _parser.getVers() + "</p>";
    body += "</body></html>";
    _response->addHeader("Content-Length", SSTR(body.size()));
    _response->endHeaders();
    _response->setBody(body);
    
    _state = SENDING_RESPONSE;
}

void Client::_buildErrorResponse(int statusCode, const std::string& message)
{
    if (_response) {
        delete _response;
    }
    
    std::string statusText = (statusCode == 400) ? "Bad Request" : 
                            (statusCode == 404) ? "Not Found" :
                            (statusCode == 500) ? "Internal Server Error" : "Error";
    
    _response = new HTTPResponse(statusCode, statusText);
    _response->addHeader("Server", "WebServ/1.0");
    _response->addHeader("Connection", "close");
    
    std::string body = "<html><body><h1>" + intToString(statusCode) + " " + statusText + "</h1>";
    body += "<p>" + message + "</p></body></html>";
    
    _response->setBody(body);
    _response->endHeaders();
    
    _state = SENDING_RESPONSE;
}

bool Client::_sendResponseChunk()
{
    char buffer[BUFF_SIZE];
    ssize_t bytesToSend = _response->readNextChunk(buffer, BUFF_SIZE);
    
    if (bytesToSend <= 0) {
        // Response complete
        if (shouldKeepAlive()) {
            _state = KEEP_ALIVE;
            reset(); // Prepare for next request
        } else {
            _state = CLOSED;
        }
        return false;
    }
    
    ssize_t bytesSent = _socket.send(buffer, bytesToSend, 0);
    
    if (bytesSent < 0) {
        _state = ERROR_STATE;
        return false;
    }
    
    // Check if response is complete
    if (_response->isComplete()) {
        if (shouldKeepAlive()) {
            _state = KEEP_ALIVE;
            reset(); // Prepare for next request
        } else {
            _state = CLOSED;
        }
        return false;
    }
    
    return true; // More data to send
}

ClientState Client::getState() const
{
    return _state;
}

bool Client::isComplete() const
{
    return _state == CLOSED || _state == KEEP_ALIVE;
}

bool Client::hasError() const
{
    return _state == ERROR_STATE;
}

bool Client::shouldKeepAlive() 
{
    // Check Connection header
    std::string connection = _parser.getHeader("Connection");
    std::transform(connection.begin(), connection.end(), connection.begin(), ::tolower);
    
    // HTTP/1.1 defaults to keep-alive, HTTP/1.0 defaults to close
    if (_parser.getVers() == "HTTP/1.1") {
        return connection != "close";
    } else {
        return connection == "keep-alive";
    }
}

void Client::reset()
{
    // Clean up current response
    if (_response) {
        delete _response;
        _response = NULL;
    }
    
    // Reset parser for next request
    _parser.reset();
    
    // Reset state
    _state = READING_REQUEST;
    memset(_readBuffer, 0, BUFF_SIZE);
}

const HTTPParser& Client::getParser() const
{
    return _parser;
}

const Socket& Client::getSocket() const
{
    return _socket;
}
