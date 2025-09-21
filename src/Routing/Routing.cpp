#include "Routing.hpp"

Routing::Routing(WebConfigFile &config) : _config(config)
{
    cout << "Server Name: [" << config.servers[0].name << "]" << endl;
}
