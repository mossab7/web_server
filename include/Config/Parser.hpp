#ifndef WEBSERV_PARSER_HPP
#define WEBSERV_PARSER_HPP

#include <map>
#include <set>
#include <vector>
#include <sstream>
#include "lexer.hpp"

// dir: {args, ...}
typedef std::pair<std::string, std::vector<std::string> > directive;

// a map of directives
typedef std::map<std::string, std::vector<std::string> > directives;

// location consist only of "dir: {args, ...}" directives
typedef directives locationConf;

// server consist only of simple directives (key:value) and block
// directives (locationConf) so if a simple directive is required
// i only look up the 'first' key in the pair
// if a server is required i look up the 'second' key
typedef std::pair<directives, std::vector<locationConf> > serverConf; 

// a list of servers configs
// this can be turned into a pair of directive and vector in
// order to support global directives
typedef std::vector<serverConf> Config;

class Parser
{   
    static directive    _parseDirective(const std::string& key, lexer& file);
    static locationConf _parseLocation(lexer& file);
    static serverConf   _parseServer(lexer& file);
    
public:
    static Config parse(const std::string& filename);
};

#endif