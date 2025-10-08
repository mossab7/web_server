#ifndef ROUTING_HPP
#define ROUTING_HPP

#include "ConfigParser.hpp"
#include "HTTPParser.hpp"

struct RouteMatch
{
    ServerConfig   *sv;
    Location       *lc;
    bool            method;
};

class Routing
{
private:
    WebConfigFile &_config;

    ServerConfig *findServer(const std::string &host);
    Location *findLocation(ServerConfig &server, const std::string &request_path);
    bool isMethodAllowed(Location &loc, const std::string &method);

public:
    Routing(WebConfigFile &config);

    RouteMatch  getMatch(const std::string &host, const std::string &request_path, const std::string &method);
};

#endif
