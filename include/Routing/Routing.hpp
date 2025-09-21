#ifndef ROUTING_HPP
#define ROUTING_HPP

#include "ConfigParser.hpp"

class Routing
{
    private:
        WebConfigFile &_config;
        
    public:
        Routing(WebConfigFile &config);
};

#endif