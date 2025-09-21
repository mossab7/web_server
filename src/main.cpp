#include "ConfigParser.hpp"
#include "Routing.hpp"
#include "HTTPParser.hpp"
#include <iostream>
#include <cassert>
#include <sstream>

int main(int ac, char **av)
{
    if (ac != 2)
    {
        cerr << "usage: ./webserv [CONFIG]" << endl;
        return (EXIT_FAILURE);
    }

    WebConfigFile config;

    if (parseConfigFile(config, av[1]))
        return (1);

    Routing routing(config);
    Server *srv = routing.findServer("localhost:8080");
    if (srv)
        cout << "Server found: " << srv->host << endl;
    else
        return (1);

    Location *loc = routing.findLocation(*srv, "/images/logo.png");
    if (loc)
        cout << "Location found: " << loc->route << endl;
    else
        return (1);

    bool allowed = routing.isMethodAllowed(*loc, "GET");
    cout << "GET allowed? " << (allowed ? "Yes" : "No") << endl;

    return (0);
}
