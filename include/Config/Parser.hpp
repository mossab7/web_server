#ifndef WEBSERV_PARSER_HPP
#define WEBSERV_PARSER_HPP

#include <vector>
#include "lexer.hpp"

struct directive_t
{
    std::string              name;
    std::vector<std::string> args;
    bool                     is_block;
    std::vector<directive_t> children;
};

class Parser
{
    // lazy lexer
    lexer       _file;  // inputfile
    int         _nestDepth; // to track how deep are we 
    directive_t _parseDirective();

    class ParserError: public std::exception
    {
        std::string     _error;
    public:
        ParserError(const std::string& msg, unsigned int line, unsigned int colm);
        ~ParserError() throw();
        const char* what() const throw();
    };

public:
    Parser(const std::string& filename);
    directive_t parse(void);
};

#endif