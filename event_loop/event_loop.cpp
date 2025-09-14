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
        for (auto &event_socket : events) 
        {
            if (event_socket == server_socket) 
            {
                Socket client_socket = server_socket.accept();
                client_socket.set_non_blocking();
                epoll.add_fd(client_socket);
                clients[client_socket.get_fd()] = new Client(client_socket);
            }
            else 
            {
                Client *client = clients[event_socket.get_fd()];
                if (event_socket & EPOLLIN)
                {
                    client->read();
                }
                else if (event_socket & EPOLLOUT)
                {
                    client->send();
                }
            }
        }
    }
}