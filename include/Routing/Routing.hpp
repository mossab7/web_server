#ifndef ROUTING_HPP
#define ROUTING_HPP

#include "ConfigParser.hpp"
#include "HTTPParser.hpp"

/**
 * @brief Holds routing match information.
 * Contains pointers to the matched server, location, and a flag for method validity.
 */
struct RouteMatch
{
    ServerConfig *sv; // Pointer to matched server configuration
    Location *lc;     // Pointer to matched location block
    bool m;           // True if the HTTP method is allowed
};

/**
 * @brief Handles HTTP request routing based on configuration.
 * Responsible for matching host, path, and method to the correct server/location.
 */
class Routing
{
private:
    WebConfigFile &_config; // Reference to global server configuration

    ServerConfig *findServer(const std::string &host);                             // Find server by host name
    Location *findLocation(ServerConfig &server, const std::string &request_path); // Find best matching location
    bool isMethodAllowed(Location &loc, const std::string &method);                // Check if method is allowed in location

public:
    Routing(WebConfigFile &config); // Initialize with configuration reference

    RouteMatch getMatch(const std::string &host, const std::string &request_path, const std::string &method); // Get matching route
};

#endif
