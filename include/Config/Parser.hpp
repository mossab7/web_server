#ifndef WEBSERV_PARSER_HPP
#define WEBSERV_PARSER_HPP

#include <map>
#include "Directive.hpp"
#include "lexer.hpp"

// ========== ParserError.hpp
class ParserError: public std::exception
{
    std::string _err;
    public:
    ParserError(const std::string& msg, uint32_t line, uint32_t colm);
    ~ParserError() throw();
    const char* what() const throw();
};

// ========== Parser.hpp
class Parser
{
    lexer _file;

    token_t         _parseArgs(directive_t& dir);
    directive_t     _parseBlock(directive_t& dir);
    directive_t     _parseDirective();

public:
    Parser(const std::string& filename);

    directive_t parse();
};

#endif