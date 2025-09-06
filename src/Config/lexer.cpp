#include "lexer.hpp"

lexer::lexer(const std::string &filepath):
    _inFile(filepath.c_str())
{
    if (!_inFile.is_open())
    {
        std::string errorMsg = "Failed to open file '" + filepath + "': " + std::strerror(errno);
        throw std::runtime_error(errorMsg);
    }
}
lexer::~lexer()
{
    if (_inFile.is_open())
        _inFile.close();
}

token_t lexer::_createToken(token_types_t t, const std::string &v = "")
{
    token_t tkn;

    tkn.type = t;
    tkn.value = v;

    return tkn;
}

void lexer::_skipUnwanted()
{
    char c;

    while ((c = _inFile.get()) != EOF)
    {
        // skip comments
        if (c == '#')
            while ((c = _inFile.get()) != EOF && c != '\n');

        // break on delimeters
        if (!std::isspace(c))
        {
            _inFile.unget();
            break;
        }
    }
}

token_t lexer::_readSymbol(char smb)
{
    char s[] = {smb};
    return _createToken((token_types_t)smb, s);
}
token_t lexer::_readQuoted(char quote)
{
    std::string value;
    char c;

    while ((c = _inFile.get()) != EOF && c != quote && c != '\n')
        value += c;

    if (c == EOF || c == '\n')
    {
        std::ostringstream error;
        error << "missing closing quote" << std::endl;
        throw std::runtime_error(error.str());
    }

    return _createToken(TKN_QUOTED, value);
}
token_t lexer::_readWord(char first)
{
    char c;
    std::string value;

    value += first;
    while ((c = _inFile.get()) != EOF)
    {
        // break on delimiters
        if (std::isspace(c) || std::strchr(SYMBOLS "'\"", c))
        {
            _inFile.unget();
            break;
        }
        value += c;
    }

    return _createToken(TKN_WORD, value);
}

token_t lexer::getNextToken(void)
{
    _skipUnwanted();

    char ch = _inFile.get();

    if (ch == EOF)
        return _createToken(TKN_EOF);

    if (std::strchr(SYMBOLS, ch))
        return _readSymbol(ch);

    if (ch == '\'' || ch == '"')
        return _readQuoted(ch);

    return _readWord(ch);
}