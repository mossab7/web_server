#include <iostream>
#include "Parser.hpp"
#include "Logger.hpp"

void print_indent(int indent) {
    for (int i = 0; i < indent; ++i) {
        std::cout << "  ";
    }
}

void print_directive(const directive_t& d, int indent = 0) {
    // Print directive name
    print_indent(indent);
    std::cout << '"' << d.name << "\": ";
    
    // Print arguments if they exist
    if (!d.args.empty()) {
        std::cout << " [";
        for (size_t i = 0; i < d.args.size(); ++i) {
            std::cout << '"' << d.args[i] << '"';
            if (i < d.args.size() - 1)
                std::cout << ", ";
        }
        std::cout << "]";
    }
    
    // Handle block directives
    if (d.is_block && d.children.size()) {
        if (!d.args.empty()) {
            std::cout << "\n";
            print_indent(indent);
        }
        std::cout << " {\n";
        
        // Print children with increased indentation
        for (size_t i = 0; i < d.children.size(); ++i) {
            print_directive(d.children[i], indent + 2);
        }
        
        print_indent(indent);
        std::cout << " }";
    }
    
    std::cout << "\n";
}
int main(int ac, char **av)
{
    (void)av;
    if (ac != 2)
    {
        std::cerr << "usage: ./webserv [CONFIG]" << std::endl;
        return 1;
    }

    Logger console;
    try {

        Parser config(av[1]);
        print_directive(config.parse(), 0);
    } catch (const std::exception& e) {
        console.error(e.what());
    }
    return 0;
}