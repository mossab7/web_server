#ifndef DIRECTIVE_HPP
#define DIRECTIVE_HPP

#include <vector>
#include <string>

// can store directives such as:
// 'root /sdf/asdf' or 'location ~ .php { ... }'
struct directive_t {
    std::string name;
    std::vector<std::string> args;
    bool is_block;
    std::vector<directive_t> children;
};

#endif
