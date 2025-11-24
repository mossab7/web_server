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

class WebConfigFile
{
private:
    std::ifstream _inputFile;
    vector<ServerConfig> _servers;

public:
    WebConfigFile(const string &fName);

    vector<ServerConfig> &getServers();


    ServerConfig getServer(const string &name);


    void addServer(const ServerConfig &server);


    ~WebConfigFile();
};




struct ServerConfig
{
    int port;
    string host;
    size_t maxBody;
    int client_timeout;
    string name;
    string root;
    vector<string> indexFiles;
    vector<Location> locations;
    map<int, string> errors;


    ServerConfig();
};





struct Location
{
    string route;
    string root;
    size_t maxBody;
    int client_timeout;
    bool autoindex;
    string cgi;
    int cgi_timeout;
    string upload;
    string redirect;
    vector<string> indexFiles;
    vector<string> methods;
    string scriptInterpreter;



    Location(ServerConfig server);
};

#endif
