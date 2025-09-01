#include "lexer.hpp"

lexer::lexer(const std::string& filepath):
    _inFile(filepath.c_str()),
    _currLine(1),
    _currColm(1),
    _scanColm(0)
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


uint32_t    lexer::getCurrLine(void) const { return _currLine; }
uint32_t    lexer::getCurrColm(void) const { return _currColm; }

token_t lexer::getNextToken(void)
{
    token_t tkn = {TKN_EOF, ""};
    char c;
    bool inWord = false;

    while ((c = _inFile.get()) != EOF)
    {
        ++_scanColm;

        // skip comments
        if (c == '#')
            while ((c = _inFile.get()) != EOF && c != '\n');

        // handle quoted strings
        if (c == '\'' || c == '"')
        {
            char quote = c;
            tkn.type = TKN_QUOTED;
            _currColm = _scanColm;
            while ((c = _inFile.get()) != EOF && c != '\n' && c != quote)
                tkn.value += c, ++_scanColm;
            if (c == EOF || c == '\n')
            {
                std::ostringstream error;
                error << _currLine << ":" << _scanColm << ":missing closing quote\n";
                throw std::logic_error(error.str());
            }
            if (c == quote) ++_scanColm;
            break;
        }

        // Handle newlines
        if (c == '\n')
        {
            if (inWord)
            {
                _inFile.unget();
                --_scanColm;
                return tkn;
            }
            ++_currLine;
            _scanColm = 0;
            continue;
        }

        // Handle symbols
        if (std::strchr(SYMBOLS, c))
        {
            if (inWord)
            {
                _inFile.unget();
                --_scanColm;
                return tkn;
            }
            _currColm = _scanColm;
            tkn.type = (token_types_t)c;
            tkn.value = c;
            return tkn;
        }

        // Handle whitespace
        if (std::isspace(c))
        {
            if (inWord) return tkn;
            continue;
        }

        // Handle word characters
        if (!inWord)
        {
            _currColm = _scanColm;
            tkn.type = TKN_WORD;
            inWord = true;
        }
        tkn.value += c;
    }

    return tkn;
}