#ifndef WEBSERV_DIRECTIVE_HPP
#define WEBSERV_DIRECTIVE_HPP

#include <string>
#include <vector>
#include <stdint.h>

enum context_t
{
    CTX_MAIN     = (1 << 0),
    CTX_SERVER   = (1 << 1),
    CTX_LOCATION = (1 << 2)
};

struct directive_t
{
    std::string              name;
    std::vector<std::string> args;
    bool                     is_block;
    std::vector<directive_t> children;
    uint32_t                 line;
    uint32_t                 colm;
};

// class IDirective
// {
//     std::string              _name;
//     std::vector<std::string> _args;
//     IDirective*              _parent;

//     uint32_t    _line;
//     uint32_t    _colm;

// public:
//     IDirective();
//     ~IDirective();

//     const std::string&               getName(void) const;
//     const std::vector<std::string>&  getArgs(void) const;

//     void    setName(const std::string& name);
//     void    setName(const std::vector<std::string>& args);

//     IDirective* getParent(void) const;
//     void        setParent(IDirective * prnt);

//     uint32_t    getLine(void) const;
//     uint32_t    getColm(void) const;

//     void        setLine(uint32_t line);
//     void        setColm(uint32_t colm);
// };

// class IBlock: public IDirective
// {
// public:
//     IBlock();
//     ~IBlock();

//     // server can have multiple 'listen' directives
//     std::vector<IDirective*>& getDirectives(const std::string& name) const;
//     std::vector<IDirective*>& getDirectives(void) const;

//     IDirective* getParent(void) const;
//     void        setParent(IDirective* dir) const;
// };

#endif