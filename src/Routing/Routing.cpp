#include "Routing.hpp"
#include <cstring>
#include <cstddef>

/**
 * @brief Routing constructor
 *
 * Stores a reference to the WebConfigFile containing all servers and locations
 * parsed from the configuration file.
 *
 * Why a reference?
 * - To operate directly on the original config without making a copy (better performance).
 * - Any access or modification to servers stays consistent with the original data.
 *
 * @param config The WebConfigFile object parsed from the config file
 */
Routing::Routing(WebConfigFile &config) : _config(config)
{
}

/**
 * @brief Finds the server that matches the given host string.
 *
 * This function iterates through all servers stored in the configuration (_config)
 * and returns a pointer to the server whose 'host' matches the provided string.
 *
 * Note:
 * - If no matching server is found, the function returns NULL.
 *
 * @param host The host string from the HTTP request to match against server hosts.
 * @return Pointer to the matching Server object, or NULL if none found.
 */
Server *Routing::findServer(const std::string &host)
{
    for (size_t i = 0; i < _config.servers.size(); i++)
    {
        if (_config.servers[i].host == host)
            return (&_config.servers[i]);
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
Location *Routing::findLocation(Server &server, const std::string &request_path)
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
