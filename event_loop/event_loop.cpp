#include "Epoll.hpp"
#include "Socket.hpp"
#include "Client.hpp"
#include <unordered_map>

void event_loop()
{
    Epoll epoll;
    Socket server_socket;
    server_socket.bind();
    server_socket.listen();
    server_socket.set_non_blocking();
    epoll.add_fd(server_socket);

    std::unordered_map<int, Client*> clients;
    while (true) 
    {
        std::vector<Socket> events = epoll.wait();
        for (size_t i = 0; i < events.size(); i++) 
        {
            try 
            {
                Socket *event_socket = &events[i];
                if (EVENT_HAS_ERROR(*event_socket)) 
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
                    
                    if (EVENT_HAS_READ(*event_socket))
                    {
                        client->read();
                        
                        // If client is ready to send, toggle to write events
                        if (client->needs_write())
                        {
                            epoll.modify_fd(*event_socket, EPOLLOUT);
                        }
                    }
                    
                    if (EVENT_HAS_WRITE(*event_socket))
                    {
                        client->send();
                        
                        // Check if we're done or need to continue writing
                        if (client->is_done())
                        {
                            epoll.remove_fd(*event_socket);
                            delete client;
                            clients.erase(event_socket->get_fd());
                        }
                        else if (client->needs_write())
                        {
                            // Keep waiting for write events
                            epoll.modify_fd(*event_socket, EPOLLOUT);
                        }
                        else
                        {
                            // Switch back to read events
                            epoll.modify_fd(*event_socket, EPOLLIN);
                        }
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
}