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

    if (!parseConfigFile(config, av[1]))
        Routing routing(config);

    return 0;
}
