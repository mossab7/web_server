#include "Parser.hpp"
#include <iostream>

Parser::ParserError::ParserError(const std::string& msg, uint32_t line, uint32_t colm)
{
    std::ostringstream error;
    error << line << ":" << colm << ": " << msg;
    _error = error.str().c_str();
}
Parser::ParserError::~ParserError() throw()
{
    
}
const char* Parser::ParserError::what() const throw()
{
    return _error.c_str();
}


Parser::Parser(const std::string& filepath):
    _file(filepath.c_str()),
    _nestDepth(0)
{

}

directive_t Parser::_parseDirective()
{
    directive_t dir;
    token_t     tkn = _file.getNextToken();

    if (tkn.type == TKN_EOF) return dir;
    if (tkn.type == TKN_RCBRAC)
    {
        if (_nestDepth > 0)
            return --_nestDepth, dir;
        else
            throw ParserError("unexpected '}' - no maching '{'",
            _file.getCurrLine(), _file.getCurrColm());
    }

    if (tkn.type != TKN_WORD)
        throw ParserError("unexpected token " + tkn.value + "'", 
            _file.getCurrLine(),
            _file.getCurrColm());

    dir.name = tkn.value;

    while ((tkn = _file.getNextToken()).type != TKN_EOF)
    {
        if (tkn.type == TKN_SEMCLN) break;
        if (tkn.type == TKN_LCBRAC) break;
        if (tkn.type != TKN_WORD && tkn.type != TKN_QUOTED && tkn.type != TKN_TILDA)
            throw ParserError("unexpected token " + tkn.value + "'", 
            _file.getCurrLine(),
            _file.getCurrColm());
        dir.args.push_back(tkn.value);
    }

    if (tkn.type == TKN_EOF)
        throw ParserError("unexpteced EOF in directive '"
            + dir.name
            + "' - (missing ';' or '{')",
             _file.getCurrLine(), _file.getCurrColm());

    if (tkn.type == TKN_LCBRAC)
    {
        dir.is_block = true;
        ++_nestDepth;
        while (true)
        {
            directive_t child = _parseDirective();
            if (child.name.empty()) break;
            dir.children.push_back(child);
        }
    }
    
    return dir;
}

directive_t Parser::parse(void)
{
    directive_t mainContext;
    directive_t tmp;

    mainContext.name = "main";
    mainContext.is_block = true;
    // keep parsing directives until no more
    while((tmp = _parseDirective()).name != "")
        mainContext.children.push_back(tmp);
    if (_nestDepth != 0)
        throw ParserError("unexpected '{' - no maching '}'",
            _file.getCurrLine(), _file.getCurrColm());
    return mainContext;
}