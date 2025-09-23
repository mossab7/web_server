#include "Epoll.hpp"
#include "Socket.hpp"
#include "Client.hpp"
#include "../utils/Logger.hpp"
#include <map>
#include <iostream>
#include <csignal>

std::string intToString(int value);

// External shutdown flag (defined in main.cpp)
extern volatile sig_atomic_t g_shutdown;

void event_loop()
{
    Epoll epoll;
    Socket server_socket;
    server_socket.bind();
    server_socket.listen();
    server_socket.set_non_blocking();
    epoll.add_fd(server_socket);

    std::map<int, Client*> clients;
    Logger logger;
    logger.info("Event loop started");
    
    while (!g_shutdown) 
    {
        std::vector<Socket> events = epoll.wait();
        for (size_t i = 0; i < events.size(); i++) 
        {
            try 
            {
                Socket *event_socket = &events[i];
                if (EVENT_HAS_ERROR(event_socket->get_event())) 
                {
                    if (*event_socket == server_socket) 
                    {
                        throw std::runtime_error("Error on server socket");
                    }
                    else 
                    {
                        Client *client = clients[event_socket->get_fd()];
                        epoll.remove_fd(*event_socket);
                        delete client;
                        clients.erase(event_socket->get_fd());
                    }
                    continue;
                }
                
                if (*event_socket == server_socket) 
                {
                    logger.debug("New incoming connection from client " + intToString(event_socket->get_fd()));
                    // Accept new connection
                    Socket client_socket = server_socket.accept();
                    client_socket.set_non_blocking();
                    epoll.add_fd(client_socket, EPOLLIN); // Start with read events
                    clients[client_socket.get_fd()] = new Client(client_socket);
                }
                else 
                {
                    // Handle client events
                    Client *client = clients[event_socket->get_fd()];
                    
                    if (EVENT_HAS_READ(event_socket->get_event()))
                    {
                        // Handle reading request data
                        bool needMoreData = client->readRequest();
                        
                        if (client->hasError()) 
                        {
                            // Error occurred, clean up client
                            epoll.remove_fd(*event_socket);
                            delete client;
                            clients.erase(event_socket->get_fd());
                            continue;
                        }
                        
                        if (!needMoreData) 
                        {
                            // Request complete, switch to write mode for response
                            if (client->getState() == SENDING_RESPONSE) 
                            {
                                epoll.modify_fd(*event_socket, EPOLLOUT);
                            }
                            else if (client->getState() == CLOSED) 
                            {
                                // Client closed connection
                                epoll.remove_fd(*event_socket);
                                delete client;
                                clients.erase(event_socket->get_fd());
                            }
                        }
                        // If needMoreData is true, keep waiting for more EPOLLIN events
                    }
                    
                    if (EVENT_HAS_WRITE(event_socket->get_event()))
                    {
                        // Handle sending response data
                        //logger.debug("Sending response to client " + intToString(event_socket->get_fd()));
                        bool needMoreWrite = client->sendResponse();
                        
                        if (client->hasError()) 
                        {
                            // Error occurred, clean up client
                            epoll.remove_fd(*event_socket);
                            delete client;
                            clients.erase(event_socket->get_fd());
                            continue;
                        }
                        
                        if (!needMoreWrite) 
                        {
                            logger.debug("Response sent to client " + intToString(event_socket->get_fd()));
                            // Response complete, check connection state
                            // if (client->getState() == CLOSED) 
                            // {
                                // Close connection
                                epoll.remove_fd(*event_socket);
                                delete client;
                                clients.erase(event_socket->get_fd());
                            // }
                            // else if (client->getState() == KEEP_ALIVE) 
                            // {
                            //     // Keep connection alive, switch back to read mode
                            //     epoll.modify_fd(*event_socket, EPOLLIN);
                            // }
                        }
                        // If needMoreWrite is true, keep waiting for more EPOLLOUT events
                    }
                }
            } 
            catch (const std::exception &e) 
            {
                // Handle exceptions (e.g., log the error)
                Socket *event_socket = &events[i];
                if (clients.find(event_socket->get_fd()) != clients.end())
                {
                    Client *client = clients[event_socket->get_fd()];
                    epoll.remove_fd(*event_socket);
                    delete client;
                    clients.erase(event_socket->get_fd());
                }
            }
        }
    }
    
    // Cleanup: Close all client connections
    logger.info("Cleaning up connections...");
    for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        epoll.remove_fd(it->first);
        delete it->second;
    }
    clients.clear();
    logger.info("Event loop shutdown complete");
}