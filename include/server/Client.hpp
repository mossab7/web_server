#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "EventHandler.hpp"
#include "Socket.hpp"
#include "../http/HTTPParser.hpp"
#include "../http/Response.hpp"
#include <sstream>
#include "FdManager.hpp"
#include "../Config/ConfigParser.hpp"


enum ClientState {
    READING_REQUEST,
    PROCESSING,
    SENDING_RESPONSE,
    KEEP_ALIVE,
    CLOSED,
    ERROR_STATE
};

class Client : public EventHandler
{
    private:
        HTTPParser _parser;
        HTTPResponse* _response;
        ClientState _state;
        char _readBuffer[BUFF_SIZE];

        // Prevent copying
        Client(const Client &other);
        Client &operator=(const Client &other);

        // Internal state management
        void _processRequest();
        void _buildErrorResponse(int statusCode, const std::string& message);
        bool _sendResponseChunk();
    public:

    Client(ServerConfig &config, FdManager &fdm);
    Client(Socket &socket, ServerConfig &config, FdManager &fdm);
    ~Client();
    
    // Main I/O operations
    bool readRequest();          // Returns true if more data expected
    bool sendResponse();         // Returns true if more data to send
    
    // State management
    ClientState getState() const;
    bool isComplete() const;     // Request fully processed and response sent
    bool hasError() const;
    bool shouldKeepAlive() ;
    
    // Reset for keep-alive connections
    void reset();
    
    // Getters for processed request data
    const HTTPParser& getParser() const;
    const Socket& getSocket() const;
    int get_fd() const;

    // EventHandler interface
    void onReadable();
    void onWritable();
    void onError();
};
#endif //CLIENT_HPP