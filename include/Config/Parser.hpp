#ifndef WEBSERV_PARSER_HPP
#define WEBSERV_PARSER_HPP

#include <vector>
#include "lexer.hpp"

struct directive_t
{
    std::string name;
    union value_t
    {
        std::string arg;
        std::vector<std::string> args;
        std::vector<directive_t> chidlren;
    } value;
};


#endif