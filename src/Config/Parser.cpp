#include "Parser.hpp"

directive Parser::_parseDirective(const std::string &key, lexer &file)
{
    directive dir;
    token_t tkn;

    dir.first = key;
    while ((tkn = file.getNextToken()).type != TKN_EOF && tkn.type != TKN_SEMCLN)
        dir.second.push_back(tkn.value);
    return dir;
}

locationConf Parser::_parseLocation(lexer &file)
{
    locationConf lc;
    token_t tkn = file.getNextToken(); // skip the '{'

    while ((tkn = file.getNextToken()).type != TKN_EOF && tkn.type != TKN_RCBRAC)
    {
        directive dir = _parseDirective(tkn.value, file);
        lc.insert(dir);
    }

    return lc;
}

serverConf Parser::_parseServer(lexer &file)
{
    serverConf srv;
    token_t tkn = file.getNextToken();

    while ((tkn = file.getNextToken()).type != TKN_EOF && tkn.type != TKN_RCBRAC)
    {
        if (tkn.value == "location")
            srv.second.push_back(_parseLocation(file));
        else
        {
            directive dir = _parseDirective(tkn.value, file);
            srv.first.insert(dir);
        }
    }
    return srv;
}

Config Parser::parse(const std::string &filename)
{
    lexer file(filename);
    Config cnf;
    token_t tkn;

    while ((tkn = file.getNextToken()).type != TKN_EOF)
    {
        if (tkn.value == "server")
            cnf.push_back(_parseServer(file));
        else
            throw std::runtime_error("Global directives are not supported");
    }

    return cnf;
}