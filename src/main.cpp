#include <iostream>
#include <csignal>
#include <exception>
#include "../include/utils/Logger.hpp"

// Forward declaration of the event_loop function
void event_loop();

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
int main(int /* argc */, char* /* argv */[])
{
    try
    {
        // Initialize logger
        Logger logger;
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
        event_loop();
        
        // This point should not be reached unless event_loop exits
        logger.info("Event loop exited");
    }
    catch (const std::exception& e)
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
// =======
// #include "ConfigParser.hpp"
// #include "Routing.hpp"
// #include "HTTPParser.hpp"
// #include "Response.hpp"
// #include "error_pages.hpp"

// int main(int ac, char **av)
// {
//     if (ac != 2)
//     {
//         cerr << "usage: ./webserv [CONFIG]" << endl;
//         return (EXIT_FAILURE);
//     }

//     initErrorPages(); 

//     WebConfigFile config;

//     if (parseConfigFile(config, av[1]))
//         return (1);

//     Routing routing(config);
//     HTTPResponse resp;

//     resp = handleRequest(routing, "localhost:8080", "/default.conf", "GET");

//     return (0);
// }
