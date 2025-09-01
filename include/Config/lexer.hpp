#ifndef WEBSERV_LEXER_HPP
#define WEBSERV_LEXER_HPP

#include <fstream>
#include <cstring>
#include <exception>
#include <string>
#include <sstream>
#include <stdint.h>

#define SYMBOLS "~;{}"

/**
 * @brief Token types for webserv configuration file parsing.
 * 
 * Defines all possible token types that can be encountered while
 * lexically analyzing a webserv configuration file. Uses character
 * values for symbols to enable direct type casting and comparison.
 */
enum token_types_t
{
    TKN_WORD    = 0,        ///< Unquoted words, identifiers, numbers (e.g., "server", "listen", "80")
    TKN_SEMCLN  = ';',      ///< Semicolon statement terminator ';'
    TKN_LCBRAC  = '{',      ///< Left curly brace '{' for block start
    TKN_RCBRAC  = '}',      ///< Right curly brace '}' for block end
    TKN_TILDA   = '~',      ///< Tilde '~' for regex patterns or special matching
    TKN_QUOTED       ,      ///< Quoted strings (e.g., 'this is a value', "file path")
    TKN_EOF              ///< End of file marker
};

/**
 * @brief Represents a single lexical token.
 * 
 * Contains both the classification (type) and content (value) of a token
 * extracted from the configuration file. Used by the parser to build
 * the configuration syntax tree.
 */
struct token_t
{
    token_types_t   type;   ///< Classification of the token
    std::string     value;  ///< String content of the token (without quotes for quoted strings)
};

/**
 * @brief Lazy lexical analyzer for webserv configuration files.
 * 
 * Implements a streaming tokenizer that processes configuration files character
 * by character, producing tokens on demand. This design provides memory efficiency
 * and immediate error detection, making it ideal for configuration file parsing.
 * 
 * Key features:
 * - Lazy evaluation: tokens generated only when requested via getNextToken()
 * - Position tracking: maintains line and column numbers for error reporting
 * - Comment support: automatically skips '#' comment lines
 * - Quoted string handling: supports both single and double quoted strings
 * - Symbol recognition: handles nginx-style configuration delimiters
 * - Whitespace normalization: properly handles spaces, tabs, and newlines
 * 
 * The lexer is designed to handle nginx-style configuration syntax:
 * @code
 * server {
 *     listen 80;
 *     server_name "example.com";
 *     root '/var/www/html';
 *     # This is a comment
 * }
 * @endcode
 * 
 * Position tracking allows for meaningful error messages like:
 * "Syntax error at line 15, column 23: unexpected character"
 */
class lexer
{
private:
    std::ifstream   _inFile;    ///< Input file stream for reading configuration data

    uint32_t    _currLine;  ///< Current line number in file (1-based, for error reporting)
    uint32_t    _currColm;  ///< Column position where current token started (1-based)
    uint32_t    _scanColm;  ///< Current scanning column position (1-based, tracks reading head)

public:
    /**
     * @brief Constructs a lexer for the specified configuration file.
     * @param filepath Path to the configuration file to tokenize
     * @throws std::runtime_error if the file cannot be opened for reading
     * 
     * Opens the configuration file and initializes position tracking.
     * The file must exist and be readable, or an exception will be thrown
     * with a descriptive error message including system error details.
     */
    lexer(const std::string& filepath);

    /**
     * @brief Destructor that ensures proper file cleanup.
     * 
     * Automatically closes the input file if it's still open,
     * preventing file descriptor leaks.
     */
    ~lexer();

    /**
     * @brief Extracts the next token from the configuration file.
     * @return A token_t structure containing the token type and value
     * 
     * Implements lazy tokenization by reading and processing characters on-demand.
     * The method handles:
     * 
     * - Word tokens: Sequences of alphanumeric characters and allowed symbols
     * - Quoted strings: Content enclosed in single or double quotes (quotes removed from value)
     * - Symbol tokens: Individual delimiters (,;{}~) 
     * - Comments: Lines starting with '#' are automatically skipped
     * - Whitespace: Used as token separators but not returned as tokens
     * - EOF: Returned when end of file is reached
     * 
     * Position tracking is automatically updated as characters are consumed,
     * enabling accurate error reporting during parsing phases.
     * 
     * Example token sequence for "server { listen 80; }":
     * 1. {TKN_WORD, "server"}
     * 2. {TKN_LCBRAC, "{"}
     * 3. {TKN_WORD, "listen"}
     * 4. {TKN_WORD, "80"}
     * 5. {TKN_SEMCLN, ";"}
     * 6. {TKN_RCBRAC, "}"}
     * 7. {TKN_EOF, ""}
     * 
     * @note Successive calls advance through the file - this is a streaming lexer
     * @note For quoted strings, the returned value excludes the surrounding quotes
     */
    token_t getNextToken(void);

    /**
     * @brief Gets the current line number for error reporting.
     * @return Current line number (1-based indexing)
     * 
     * Useful for generating meaningful error messages that help users
     * locate syntax errors in their configuration files.
     */
    uint32_t getCurrLine(void) const;

    /**
     * @brief Gets the column position where the current token started.
     * @return Column number where the current token began (1-based indexing)
     * 
     * Returns the column position where the most recently returned token
     * started, not the current scanning position. Useful for pinpointing
     * the exact location of problematic tokens in error messages.
     */
    uint32_t getCurrColm(void) const;
};

#endif