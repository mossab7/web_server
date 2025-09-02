#include "lexer.hpp"

lexer::lexer(const std::string &filepath):
    _inFile(filepath.c_str()),
    _currLine(1),
    _currColm(1),
    _scanColm(1)
{
    if (!_inFile.is_open())
    {
        std::string errorMsg = "Failed to open log file '" + filepath + "': " + std::strerror(errno);
        throw std::runtime_error(errorMsg);
    }
}
lexer::~lexer()
{
    if (_inFile.is_open())
        _inFile.close();
}

uint32_t lexer::getCurrLine(void) const { return _currLine; }
uint32_t lexer::getCurrColm(void) const { return _currColm; }

token_t lexer::_createToken(token_types_t t, const std::string &v = "")
{
    token_t tkn;

    tkn.colm = _currColm;
    tkn.line = _currLine;
    tkn.type = t;
    tkn.value = v;

    return tkn;
}

char lexer::_getChar()
{
    char c = _inFile.get();

    if (c != EOF)
    {
        ++_scanColm;
        if (c == '\n')
        {
            ++_currLine;
            _scanColm = 1;
        }
    }
    return c;
}
void lexer::_ungetChar()
{
    --_scanColm;
    _inFile.unget();
}

void lexer::_skipUnwanted()
{
    char c;

    while ((c = _getChar()) != EOF)
    {
        // skip comments
        if (c == '#')
            while ((c = _getChar()) != EOF && c != '\n')
                ;

        // break on delimeters
        if (!std::isspace(c))
        {
            _ungetChar();
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

    // for error reporting
    uint32_t tmpline = _currLine;
    uint32_t tmpcolm = _currColm;

    while ((c = _getChar()) != EOF && c != quote && c != '\n')
        ++tmpcolm, value += c;

    if (c == EOF || c == '\n')
    {
        std::ostringstream error;
        error << tmpline << ":" << tmpcolm << ": "
              << "missing closing quote" << std::endl;
        throw std::runtime_error(error.str());
    }

    return _createToken(TKN_QUOTED, value);
}
token_t lexer::_readWord(char first)
{
    char c;
    std::string value;

    value += first;
    while ((c = _getChar()) != EOF)
    {
        // break on delimiters
        if (std::isspace(c) || std::strchr(SYMBOLS "'\"", c))
        {
            _ungetChar();
            break;
        }
        value += c;
    }

    return _createToken(TKN_WORD, value);
}

token_t lexer::getNextToken(void)
{
    _skipUnwanted();

    char ch = _getChar();

    if (ch == EOF)
        return _createToken(TKN_EOF);

    _currColm = _scanColm - 1;

    if (std::strchr(SYMBOLS, ch))
        return _readSymbol(ch);

    if (ch == '\'' || ch == '"')
        return _readQuoted(ch);

    return _readWord(ch);
    ;
}