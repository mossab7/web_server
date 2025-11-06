#include "../../include/server/Server.hpp"
#include "../../include/server/Client.hpp"
#include "../../include/utils/Logger.hpp"
#include <sstream>

// Helper macro for converting to string
#define SSTR(x) static_cast<std::ostringstream &>((std::ostringstream() << x)).str()

Server::Server(ServerConfig &config, FdManager &fdm) 
    : EventHandler(config, fdm, -1) 
{
    Logger logger;
    
    // Set _socket options for reuse
    int opt = 1;
    if (setsockopt(_socket.get_fd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set SO_REUSEADDR");
    }
    
    // Bind to address
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(config.host.c_str());
    address.sin_port = htons(config.port);
    
    _socket.bind(address);
    _socket.listen();
    _socket.set_non_blocking();
    
    logger.info("Server initialized on " + config.host + ":" + SSTR(config.port));
}

Server::~Server()
{
}

int Server::get_fd() const
{
    return _socket.get_fd();
}

void Server::onEvent(uint32_t events)
{
    if (IS_ERROR_EVENT(events)) {
        onError();
        return;
    }
    if (IS_READ_EVENT(events))
        onReadable();
    if (IS_WRITE_EVENT(events)) 
        onWritable();
    if (IS_TIMEOUT_EVENT(events))
        onTimeout();
}

void Server::onReadable()
{
    Logger logger;
    
    try {
        // Accept new connection
        int client_socket = _socket.accept();
        
        logger.info("New client connection accepted on fd: " + SSTR(client_socket));

        // Create new Client handler
        Client* client = new Client(client_socket, _config, _fd_manager);
        
        // Register client with epoll for reading (Level-Triggered)
        _fd_manager.add(client->get_fd(), client, READ_EVENT);
        
    } catch (const std::exception& e) {
        logger.error("Failed to accept client connection: " + std::string(e.what()));
        // Don't throw - server should continue accepting other connections
    }
}

void Server::onWritable()
{
    // Server socket should never be in writable state
    Logger logger;
    logger.warning("Unexpected writable event on server socket");
}

void Server::onError()
{
    Logger logger;
    logger.error("Error on server socket fd: " + SSTR(get_fd()));
    
    _fd_manager.remove(get_fd());
}

int Server::get_fd()
{
    return (_socket.get_fd());
}


void Server::destroy()
{
    delete this;
}

void Server::onTimeout()
{
    // Server socket should not have timeouts
    Logger logger;
    logger.warning("Unexpected timeout event on server socket");
}
