#include "Routing.hpp"

RouteMatch::RouteMatch()
    : location(NULL),
      isMatched(false),
      methodAllowed(false),
      isCGI(false),
      isRedirect(false),
      isDirectory(false),
      autoIndex(false),
      maxBodySize(0)
{
}

bool RouteMatch::isValidMatch() const
{
    return (isMatched && location != NULL);
}

bool RouteMatch::isUploadAllowed() const
{
    return (!uploadDir.empty());
}

Routing::Routing(ServerConfig &server) : _server(server)
{
}

Location *Routing::_findLocation(const string &path)
{
    (void)path;
    return (NULL);
}

bool Routing::_matchesRoute(const string &path, const string &route)
{
    (void)path;
    (void)route;
    return (false);
}

string Routing::_resolvePath(Location &loc, const string &reqPath)
{
    (void)loc;
    (void)reqPath;
    return (string());
}

string Routing::_cleanPath(const string &path)
{
    (void)path;
    return (string());
}

string Routing::_joinPath(const string &base, const string &path)
{
    (void)base;
    (void)path;
    return (string());
}

string Routing::_getRelativePath(const string &path, const string &route)
{
    (void)path;
    (void)route;
    return (string());
}

bool Routing::_isCGI(Location &loc)
{
    (void)loc;
    return (false);
}

void Routing::_splitCGIPath(const string &fsPath, string &scriptPath, string &pathInfo)
{
    (void)fsPath;
    (void)scriptPath;
    (void)pathInfo;
}

bool Routing::_isMethodAllowed(Location &loc, const string &method)
{
    (void)loc;
    (void)method;
    return (false);
}

bool Routing::_isPathExists(const string &path)
{
    (void)path;
    return (false);
}

bool Routing::_isDirectory(const string &path)
{
    (void)path;
    return (false);
}

bool Routing::_isFile(const string &path)
{
    (void)path;
    return (false);
}

string Routing::_getRoot(Location &loc)
{
    (void)loc;
    return (string());
}

size_t Routing::_getMaxBodySize(Location &loc)
{
    (void)loc;
    return size_t();
}

vector<string> Routing::_getIndexFiles(Location &loc)
{
    (void)loc;
    return (vector<string>());
}

RouteMatch Routing::match(const string &path, const string &method)
{
    (void)path;
    (void)method;
    return (RouteMatch());
}

string Routing::getErrorPage(int code)
{
    (void)code;
    return (string());
}

string Routing::getAllowedMethodsStr(Location &loc)
{
    (void)loc;
    return (string());
}
