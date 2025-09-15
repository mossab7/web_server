#include <iostream>
#include <algorithm>
#include <arpa/inet.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>

using namespace std;

class   WebConfigFile;
class   Server;
class   Location;

/**
 * @class WebConfigFile
 * @brief Represents the full web server configuration.
 *
 * Stores all server blocks defined in a configuration file, 
 * including each server's associated location blocks.
 *
 * @var servers
 * @brief Container holding all Server objects parsed from the configuration.
 */
class WebConfigFile {
    public:
        vector<Server> servers;
};

/**
 * @class Server
 * @brief Holds all config for one server block.
 *
 * Includes IP, port, name, root, client body limit, index files,
 * error pages, and its locations.
 */
class Server {
    public:
        int port;
        string ip;
        size_t maxBody;
        string name;
        string root;
        vector<string> files;
        vector<Location> locations;
        map<int, string> errors;

        Server();
};

/**
 * @class Location
 * @brief Holds all config for a location block within a server.
 *
 * Stores path, root, client body limit, index files, allowed methods,
 * autoindex flag, CGI path, redirects, and upload folder.
 */
class Location {
    public:
        string route;
        string root;
        size_t maxBody;
        bool autoindex;
        string cgi;
        string upload;
        string redirect;
        vector<string> files;
        vector<string> methods;

        void ApplyDefaults(Server server);
};

string trim(const string &str);
string reduceSpaces(const string &str);
string removeComment(const string &str);
vector<string> split(const string &str);
short parseConfigFile(WebConfigFile &config, const string &fname);
short printError(string &str, const string &fname, size_t &lnNbr);
size_t myAtol(string str, string &line, const string &fname, size_t &lnNbr);
short handleDirective(string &str, const string &fname, size_t &lnNbr, WebConfigFile &config);
short handleServer(string str, vector<string> &tokens, Server &srvTmp, const string &fname, size_t &lnNbr);
short handleLocation(string str, vector<string> &tokens, Location &locTmp, const string &fname, size_t &lnNbr);