#include "ConfigParser.hpp"
#include "Routing.hpp"
#include "HTTPParser.hpp"
#include "Response.hpp"
#include "error_pages.hpp"

int main(int ac, char **av)
{
    if (ac != 2)
    {
        cerr << "usage: ./webserv [CONFIG]" << endl;
        return (EXIT_FAILURE);
    }

    initErrorPages(); 

    WebConfigFile config;

    if (parseConfigFile(config, av[1]))
        return (1);

    Routing routing(config);
    HTTPResponse resp;

    resp = handleRequest(routing, "localhost:8080", "/default.conf", "GET");

    return (0);
}
