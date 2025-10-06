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

void Client::onReadable()
{
    Logger logger;
    
    try {
        bool needMoreData = readRequest();

        if (hasError())
        {
            logger.error("Client error on fd " + SSTR(get_fd()) + ", closing connection");
            // Error state, close connection and remove from FdManager
            _fd_manager.remove(get_fd());
            delete this;
            return;
        }
        
        if (!needMoreData)
        {
            if (_state == SENDING_RESPONSE)
            {
                // Switch to writable to send response (Level-Triggered)
                _fd_manager.modify(this, EPOLLOUT);
                return;
            }
            else if (_state == CLOSED)
            {
                logger.info("Client closed connection on fd " + SSTR(get_fd()));
                // Connection closed by client, cleanup
                _fd_manager.remove(get_fd());
                delete this;
                return;
            }
        }
    } catch (const std::exception& e) {
        logger.error("Exception in Client::onReadable on fd " + SSTR(get_fd()) + ": " + std::string(e.what()));
        _fd_manager.remove(get_fd());
        delete this;
    }
}

void Client::onWritable()
{
    Logger logger;
    
    try {
        bool needMoreWrite = sendResponse();

        if (hasError())
        {
            logger.error("Client send error on fd " + SSTR(get_fd()) + ", closing connection");
            _fd_manager.remove(get_fd());
            delete this;
            return;
        }
        
        if (!needMoreWrite)
        {
            if (_state == CLOSED)
            {
                logger.info("Response sent, closing connection on fd " + SSTR(get_fd()));
                _fd_manager.remove(get_fd());
                delete this;
                return;
            }
            else if (_state == KEEP_ALIVE)
            {
                logger.info("Keep-alive connection on fd " + SSTR(get_fd()) + ", switching to read mode");
                // Reset for next request
                reset();
                // Switch back to readable (Level-Triggered)
                _fd_manager.modify(this, EPOLLIN);
            }
        }
    } catch (const std::exception& e) {
        logger.error("Exception in Client::onWritable on fd " + SSTR(get_fd()) + ": " + std::string(e.what()));
        _fd_manager.remove(get_fd());
        delete this;
    }
}

void Client::onError()
{
    Logger logger;
    logger.error("Error event on client fd " + SSTR(get_fd()));
    _fd_manager.remove(get_fd());
    delete this; 
}

Client::Client(ServerConfig &config, FdManager &fdm) : EventHandler(config, fdm), _response(NULL), _state(READING_REQUEST) 
{
    memset(_readBuffer, 0, BUFF_SIZE);
}

Client::Client(Socket &socket, ServerConfig &config, FdManager &fdm) : EventHandler(config, fdm, socket), _response(NULL), _state(READING_REQUEST) 
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
    Logger logger;
    
    if (_state != READING_REQUEST) {
        return false;
    }
    
    // Read data from socket
    ssize_t bytesRead = _socket.recv(_readBuffer, BUFF_SIZE - 1, 0);
    
    if (bytesRead < 0) {
        // Error reading
        logger.error("Read error on fd " + SSTR(get_fd()));
        _state = ERROR_STATE;
        return false;
    }
    
    if (bytesRead == 0) {
        // Client closed connection
        logger.info("Client closed connection on fd " + SSTR(get_fd()));
        _state = CLOSED;
        return false;
    }
    
    // Null terminate for safety
    _readBuffer[bytesRead] = '\0';
    
    // Feed chunk to parser
    _parser.addChunk(_readBuffer, bytesRead);
    
    // Check parser state
    if (_parser.isError()) {
        logger.warning("Parse error on fd " + SSTR(get_fd()));
        _state = ERROR_STATE;
        _buildErrorResponse(400, "Bad Request");
        return false;
    }
    
    if (_parser.isComplete()) {
        logger.info("Request complete on fd " + SSTR(get_fd()));
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
    Logger logger;
    char buffer[BUFF_SIZE];
    ssize_t bytesToSend = _response->readNextChunk(buffer, BUFF_SIZE);
    
    if (bytesToSend <= 0) {
        // Response complete
        if (shouldKeepAlive()) {
            _state = KEEP_ALIVE;
        } else {
            _state = CLOSED;
        }
        return false;
    }
    
    ssize_t bytesSent = _socket.send(buffer, bytesToSend, 0);
    
    if (bytesSent < 0) {
        // Error sending
        logger.error("Send error on fd " + SSTR(get_fd()));
        _state = ERROR_STATE;
        return false;
    }
    
    if (bytesSent < bytesToSend) {
        // Partial send - with level-triggered mode, we'll be called again
        // Need to track offset for next send
        logger.warning("Partial send on fd " + SSTR(get_fd()) + ": " + SSTR(bytesSent) + "/" + SSTR(bytesToSend));
        return true;
    }
    
    // Check if response is complete
    if (_response->isComplete()) {
        logger.info("Response complete on fd " + SSTR(get_fd()));
        if (shouldKeepAlive()) {
            _state = KEEP_ALIVE;
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

int Client::get_fd() const
{
    return _socket.get_fd();
}

