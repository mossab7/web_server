#ifndef WEBSERV_LEXER_HPP
#define WEBSERV_LEXER_HPP

#include <fstream>
#include <cstring>
#include <exception>
#include <string>
#include <sstream>
#include <stdint.h>

#define SYMBOLS ";{}"

/**
 * @brief Token types for webserv configuration parsing.
 * Defines possible token types encountered during lexical analysis.
 */
enum token_types_t
{
    TKN_WORD    = 0,
    TKN_SEMCLN  = ';',
    TKN_LCBRAC  = '{',
    TKN_RCBRAC  = '}',
    TKN_QUOTED,             ///< Quoted strings
    TKN_EOF
};

/**
 * @brief Represents a single lexical token.
 */
struct token_t
{
    token_types_t   type;   ///< Token classification
    std::string     value;  ///< Token content (quotes removed if quoted)
};

/**
 * @brief Lazy lexer for webserv configuration files.
 * Reads the input file and produces tokens on demand.
 */
class lexer
{
private:
    std::ifstream   _inFile;    ///< Input file stream

    /**
     * @brief Creates a token with current position information.
     * @param type Token type
     * @param value Token value
     * @return Token with position set
     */
    token_t     _createToken(token_types_t type, const std::string& value);
    
    /**
     * @brief Skips whitespace and comments.
     */
    void        _skipUnwanted();

    /**
     * @brief Creates a symbol token.
     * @param symbol The symbol character
     * @return Symbol token
     */
    token_t     _readSymbol(char smb);

    /**
     * @brief Reads a quoted string token.
     * @param quote The quote character (' or ")
     * @return Quoted string token
     * @throws std::logic_error on unterminated string
     */
    token_t     _readQuoted(char quote);

    /**
     * @brief Reads a word token.
     * @param f First character of the word
     * @return Word token
     */
    token_t     _readWord(char f);
public:
    /**
     * @brief Constructs a lexer for a configuration file.
     * @param filepath Path to the file to tokenize
     * @throws std::runtime_error if file cannot be opened
     */
    lexer(const std::string& filepath);

    /**
     * @brief Closes the input file on destruction.
     */
    ~lexer();

    /**
     * @brief Returns the next token from the input.
     * @return Next token (type and value)
     */
    token_t getNextToken(void);
};


#endif