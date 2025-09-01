#ifndef WEBSERV_PARSER_HPP
#define WEBSERV_PARSER_HPP

#include "lexer.hpp"
#include "directive.hpp"

/**
 * @class Parser
 * @brief Parses an input file to extract and interpret directives.
 */
class Parser
{
    lexer _file; /**< Input file lexer. */
    int _nestDepth; /**< Nesting depth counter. */

    /**
     * @brief Parses a single directive from the input.
     * @return Parsed directive.
     */
    directive_t _parseDirective(); /**< Parses one directive. */

    /**
     * @class ParserError
     * @brief Exception class for handling parsing errors.
     */
    class ParserError: public std::exception
    {
        std::string _error; /**< Error message. */

    public:
        /**
         * @brief Constructs a ParserError with a message, line, and column.
         * @param msg Error message.
         * @param line Line number where error occurred.
         * @param colm Column number where error occurred.
         */
        ParserError(const std::string& msg, uint32_t line, uint32_t colm);
        ~ParserError() throw();
        const char* what() const throw();
    };

public:
    /**
     * @brief Constructs a Parser for the given file.
     * @param filename Name of the input file.
     */
    Parser(const std::string& filename);

    /**
     * @brief Parses the entire input file and returns the parsed directives.
     * @return Parsed directives.
     */
    directive_t parse(void); /**< Parses the input file. */
};


#endif