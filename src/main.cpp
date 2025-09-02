#include <iostream>
#include "Logger.hpp"
#include "lexer.hpp"

int main(int ac, char **av)
{
    (void)av;
    if (ac != 2)
    {
        std::cerr << "usage: ./webserv [CONFIG]" << std::endl;
        return 1;
    }

    // Logger console;
    // try {

    //     Parser config(av[1]);
    //     print_directive(config.parse(), 0);
    // } catch (const std::exception& e) {
    //     console.error(e.what());
    // }

    lexer lx(av[1]);
    token_t t;

    while ((t = lx.getNextToken()).type != TKN_EOF)
    {
        std::cout << t.line << ":" << t.colm << " -> " << t.value << std::endl;
        // break;
    }
    return 0;
}