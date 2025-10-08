#include "Routing.hpp"

/**
 * @brief Constructs a Routing object with a reference to a WebConfigFile.
 *
 * This constructor initializes the Routing instance by storing a reference
 * to the configuration object (_config), which holds all server and location
 * information. The Routing object will use this configuration to route
 * HTTP requests to the appropriate server.
 *
 * @param config Reference to the WebConfigFile containing server configurations.
 */
Routing::Routing(WebConfigFile &config) : _config(config)
{
}

/**
 * @brief Finds and returns a pointer to the server that matches the given host name.
 *
 * This function searches through all servers in the configuration (_config)
 * and returns a pointer to the ServerConfig whose 'name' matches the provided host string.
 * If no matching server is found, it returns NULL.
 *
 * @param host The host string to match against the server names.
 * @return Pointer to the matching ServerConfig object, or NULL if no match is found.
 */
ServerConfig *Routing::findServer(const std::string &host)
{
    std::vector<ServerConfig> &servers = _config.getServers();
    for (size_t i = 0; i < servers.size(); i++)
    {
        if (servers[i].name == host)
            return &servers[i];
    }
    return (NULL);
}

/**
 * @brief Finds the Location that best matches the request path.
 *
 * Returns the Location with the longest route that matches the beginning
 * of the request path. If no match is found, returns NULL.
 *
 * @param server The server containing the locations.
 * @param request_path The HTTP request path to match.
 * @return Pointer to the best matching Location, or NULL if none found.
 */
Location *Routing::findLocation(ServerConfig &server, const std::string &request_path)
{
    Location *best = NULL;
    size_t best_len = 0;

    for (size_t i = 0; i < server.locations.size(); i++)
    {
        Location &loc = server.locations[i];
        if (request_path.compare(0, loc.route.length(), loc.route) == 0)
        {
            if (loc.route.length() > best_len)
            {
                best = &loc;
                best_len = loc.route.length();
            }
        }
    }
    return (best);
}

/**
 * @brief Checks if the given HTTP method is allowed for a specific Location.
 *
 * This function iterates through all methods defined in the Location object
 * and returns true if the provided method matches any of the allowed methods.
 *
 * @param loc The Location object containing allowed HTTP methods.
 * @param method The HTTP method from the client request to check.
 * @return true if the method is allowed for this Location, false otherwise.
 */
bool Routing::isMethodAllowed(Location &loc, const std::string &method)
{
    for (size_t i = 0; i < loc.methods.size(); i++)
    {
        if (loc.methods[i] == method)
            return (true);
    }
    return (false);
}

RouteMatch Routing::getMatch(const std::string &host, const std::string &request_path, const std::string &method)
{
    RouteMatch res;

    res.sv = NULL;
    res.lc = NULL;
    res.method = false;

    if (!(res.sv = findServer(host)))
        return (res);

    if (!(res.lc = findLocation(*res.sv, request_path)))
        return (res);

    res.method = isMethodAllowed(*res.lc, method);

    return (res);
}
