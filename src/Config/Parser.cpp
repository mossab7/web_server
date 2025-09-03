#include "Parser.hpp"

// =======
ParserError::ParserError(const std::string& msg, uint32_t line, uint32_t colm)
{
    std::ostringstream err;

    err << line << ":" << colm << ": " << msg;
    _err = err.str();
}
ParserError::~ParserError() throw() {}
const char* ParserError::what() const throw()
{
    return _err.c_str();
}



// =============
Parser::Parser(const std::string& filename):
    _file(filename.c_str())
{

}

token_t Parser::_parseArgs(directive_t& dir)
{
    token_t tkn;

    while ((tkn = _file.getNextToken()).type != TKN_EOF)
    {
        if (tkn.type == TKN_SEMCLN || tkn.type == TKN_LCBRAC) break;
        dir.args.push_back(tkn.value);
    }

    return tkn;
}

directive_t Parser::_parseBlock(directive_t& dir)
{
    dir.is_block = true;

    while (true)
    {
        directive_t child = _parseDirective();
        if (child.name.empty())
            break;
        dir.children.push_back(child);
    }

    return dir;
}

directive_t Parser::_parseDirective()
{
    directive_t dir;
    token_t     tkn = _file.getNextToken();

    if (tkn.type == TKN_EOF || tkn.type == TKN_RCBRAC)
        return dir;
    
    if (tkn.type != TKN_WORD)
        throw ParserError("unexpected token '" + tkn.value + "'", tkn.line, tkn.colm);
    
    dir.name = tkn.value;
    dir.line = tkn.line;
    dir.colm = tkn.colm;

    token_t terminator = _parseArgs(dir);
    
    if (terminator.type == TKN_LCBRAC)
        _parseBlock(dir);
    
    return dir;
}

directive_t Parser::parse(void)
{
    directive_t main;
    
    main.name = "main";
    main.is_block = true;
    
    while (true)
    {
        directive_t child = _parseDirective();
        if (child.name.empty())
            break;
        main.children.push_back(child);
    }

    return main;
}