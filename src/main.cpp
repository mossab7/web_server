#include <iostream>
#include <csignal>
#include <exception>
#include "ConfigParser.hpp"
#include "Routing.hpp"
#include "HTTPParser.hpp"
#include "Response.hpp"
#include "../include/utils/Logger.hpp"
#include "Epoll.hpp"
#include "Socket.hpp"
#include "Client.hpp"
#include "../utils/Logger.hpp"
#include "FdManager.hpp"
#include <map>
#include <iostream>
#include <csignal>
#include "EventLoop.hpp"
#include "Server.hpp"

std::string intToString(int value);

// External shutdown flag (defined in main.cpp)

// Forward declaration of the event_loop function

// Global flag for graceful shutdown
volatile sig_atomic_t g_shutdown = 0;

/**
 * @brief Signal handler for graceful shutdown
 * @param signal The signal number
 */
void signal_handler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        std::cout << "\nReceived shutdown signal. Stopping server..." << std::endl;
        g_shutdown = 1;
    }
}

/**
 * @brief Setup signal handlers for graceful shutdown
 */
void setup_signal_handlers()
{
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        throw std::runtime_error("Failed to setup SIGINT handler");
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
        throw std::runtime_error("Failed to setup SIGTERM handler");
    }

    // Ignore SIGPIPE to handle broken pipe errors gracefully
    signal(SIGPIPE, SIG_IGN);
}

/**
 * @brief Main function to test the event loop
 * @param argc Number of command line arguments (unused)
 * @param argv Command line arguments (unused)
 * @return Exit status
 */
int main(int ac, char **av)
{
    if (ac != 2)
    {
        cerr << "usage: ./webserv [CONFIG]" << endl;
        return (EXIT_FAILURE);
    }

    try
    {

        WebConfigFile config(av[1]);

        // Initialize logger
        Logger logger;
        EventLoop eventLoop;
        std::vector<ServerConfig> servers = config.getServers();

        // Store server pointers for proper lifetime management
        std::vector<Server *> serverInstances;

        for (std::vector<ServerConfig>::iterator it = servers.begin(); it != servers.end(); ++it)
        {
            Server *server = new Server(*it, eventLoop.fd_manager);
            serverInstances.push_back(server);
            eventLoop.fd_manager.add(server->get_fd(), server, EPOLLIN);
            logger.info("Configured server: " + it->name + " on " + it->host + ":" + intToString(it->port));
        }
        logger.info("Starting webserver...");

        // Setup signal handlers for graceful shutdown
        setup_signal_handlers();
        logger.info("Signal handlers configured");

        // Print startup message
        std::cout << "=== Webserver Starting ===" << std::endl;
        std::cout << "Press Ctrl+C to stop the server gracefully" << std::endl;
        std::cout << "Listening for connections..." << std::endl;

        // Start the event loop
        logger.info("Starting event loop");
        eventLoop.run();

        // Cleanup servers after event loop exits
        logger.info("Cleaning up servers...");
        for (std::vector<Server *>::iterator it = serverInstances.begin(); it != serverInstances.end(); ++it)
        {
            eventLoop.fd_manager.remove((*it)->get_fd());
            delete *it;
        }

        // This point should not be reached unless event_loop exits
        logger.info("Event loop exited");
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }

    std::cout << "Server stopped gracefully" << std::endl;
    return 0;
}
