#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <iostream>
#include <algorithm>
#include <arpa/inet.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "SpecialResponse.hpp"

using namespace std;

class WebConfigFile;
struct ServerConfig;
struct Location;

/**
 * @brief Represents the configuration of the entire web server setup.
 *
 * Stores all server definitions and provides methods to access and modify them.
 */
class WebConfigFile
{
private:
    std::ifstream _inputFile;      ///< Input file stream for reading the config file.
    vector<ServerConfig> _servers; ///< List of all servers in the configuration.

public:
    /**
     * @brief Constructs a WebConfigFile from a configuration file.
     * @param fName Path to the configuration file.
     */
    WebConfigFile(const string &fName);

    /**
     * @brief Returns a reference to the vector of all servers.
     */
    vector<ServerConfig> &getServers();

    /**
     * @brief Returns a server by its name.
     * @param name Name of the server.
     * @return Server object (default if not found).
     */
    ServerConfig getServer(const string &name);

    /**
     * @brief Adds a server to the configuration.
     * @param server Server object to add.
     */
    void addServer(const ServerConfig &server);

    /**
     * @brief Destructor that closes the configuration file if it is still open.
     */
    ~WebConfigFile();
};

/**
 * @brief Represents a single server configuration.
 *
 * Contains network info, root directory, max body size, error pages,
 * and its associated locations.
 */
struct ServerConfig
{
    int port;                   ///< Server port number.
    string host;                ///< Server host address.
    size_t maxBody;             ///< Maximum allowed body size.
    string name;                ///< Server name.
    string root;                ///< Root directory.
    vector<string> indexFiles;  ///< Default index files.
    vector<Location> locations; ///< Location-specific configurations.
    map<int, string> errors;    ///< Custom error pages.

    /**
     * @brief Default constructor initializing default values.
     */
    ServerConfig();
};

/**
 * @brief Represents a location block within a server.
 *
 * Contains settings like root, allowed methods, redirections, CGI,
 * autoindex, and uploaded file storage.
 */

struct Location
{
    string route;              ///< Route path for this location.
    string root;               ///< Root directory override.
    size_t maxBody;            ///< Maximum allowed body size for this location.
    bool autoindex;            ///< Whether directory listing is enabled.
    string cgi;                ///< CGI script path.
    int cgi_timeout;           ///< Timeout for CGI execution.
    string upload;             ///< Upload directory path.
    string redirect;           ///< Redirect URL.
    vector<string> indexFiles; ///< Default index files for this location.
    vector<string> methods;    ///< Allowed HTTP methods (GET, POST, DELETE).
    string scriptInterpreter;  ///< Path to the interpreter used for executing script files (e.g. /usr/bin/python3).

    /**
     * @brief Constructs a Location with default values from a Server.
     * @param server Server object to apply defaults from.
     */
    Location(ServerConfig server);
};

#endif
