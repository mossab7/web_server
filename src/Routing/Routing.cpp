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

RouteMatch Routing::match(const string &path, const string &method)
{
    RouteMatch result;

    Location *loc = _findLocation(path);
    if (!loc)
        return (result);

    result.location = loc;
    result.isMatched = true;

    result.methodAllowed = _isMethodAllowed(*loc, method);
    if (!result.methodAllowed)
        return (result);

    result.fsPath = _resolvePath(*loc, path);

    result.isCGI = _isCGI(*loc);
    result.isRedirect = !loc->redirect.empty();
    result.isDirectory = _isDirectory(result.fsPath);
    result.isFile = _isFile(result.fsPath);
    result.doesExist = _isPathExists(result.fsPath);

    result.autoIndex = loc->autoindex;
    result.uploadDir = loc->upload;
    result.redirectUrl = loc->redirect;
    result.maxBodySize = _getMaxBodySize(*loc);
    result.indexFiles = _getIndexFiles(*loc);

    if (result.isCGI)
    {
        _splitCGIPath(result.fsPath, result.scriptPath, result.pathInfo);
        result.scriptInterpreter = loc->scriptInterpreter;
    }

    return (result);
}

string Routing::getErrorPage(int code)
{
    if (_server.errors.find(code) != _server.errors.end())
        return (_server.errors[code]);

    return ("");
}

string Routing::getAllowedMethodsStr(Location &loc)
{
    if (loc.methods.empty())
        return ("GET, POST, DELETE");

    std::string res;
    for (size_t i = 0; i < loc.methods.size(); ++i)
    {
        res += loc.methods[i];
        if (i < loc.methods.size() - 1)
            res += ", ";
    }

    return (res);
}

Location *Routing::_findLocation(const string &path)
{
    Location *bestMatch = NULL;
    size_t bestLen = 0;

    for (size_t i = 0; i < _server.locations.size(); ++i)
    {
        Location &loc = _server.locations[i];
        if (_matchesRoute(path, loc.route) && loc.route.length() > bestLen)
        {
            bestMatch = &loc;
            bestLen = loc.route.length();
        }
    }
    return (bestMatch);
}

bool Routing::_matchesRoute(const string &path, const string &route)
{
    if (route == "/")
        return (true);

    if (path.compare(0, route.size(), route) == 0)
    {
        if (path.size() == route.size() || path[route.size()] == '/')
            return (true);
    }

    return (false);
}

string Routing::_resolvePath(Location &loc, const string &reqPath)
{
    std::string root = _getRoot(loc);
    std::string relative = _getRelativePath(reqPath, loc.route);
    std::string full = _joinPath(root, relative);
    return (_cleanPath(full));
}

string Routing::_cleanPath(const string &path)
{
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string item;

    while (std::getline(ss, item, '/'))
    {
        if (item == "")
            continue;
        if (item == ".." && !parts.empty())
            parts.pop_back();
        else if (item != "..")
            parts.push_back(item);
    }

    std::string normalized = "";
    for (size_t i = 0; i < parts.size(); ++i)
    {
        normalized += parts[i];
        if (i < parts.size() - 1)
            normalized += "/";
    }
    return (normalized);
}

string Routing::_joinPath(const string &base, const string &path)
{
    if (base.empty())
        return (path);

    if (path.empty())
        return (base);

    if (base[base.size() - 1] == '/')
        return (base + (path[0] == '/' ? path.substr(1) : path));

    return (base + (path[0] == '/' ? path : "/" + path));
}

string Routing::_getRelativePath(const string &path, const string &route)
{
    if (path.compare(0, route.size(), route) == 0)
        return (path.substr(route.size()));

    return (path);
}

bool Routing::_isCGI(Location &loc)
{
    return (!loc.cgi.empty());
}

void Routing::_splitCGIPath(const string &fsPath, string &scriptPath, string &pathInfo)
{
    std::string tmp = fsPath;
    while (!tmp.empty() && !_isFile(tmp))
    {
        size_t pos = tmp.find_last_of('/');
        if (pos == std::string::npos)
            break;
        tmp = tmp.substr(0, pos);
    }
    scriptPath = tmp;
    pathInfo = fsPath.substr(tmp.length());
}

bool Routing::_isMethodAllowed(Location &loc, const string &method)
{
    if (loc.methods.empty())
        return (true);

    for (size_t i = 0; i < loc.methods.size(); ++i)
    {
        if (loc.methods[i] == method)
            return (true);
    }

    return (false);
}

bool Routing::_isPathExists(const string &path)
{
    struct stat st;
    return (stat(path.c_str(), &st) == 0);
}

bool Routing::_isDirectory(const string &path)
{
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
}

bool Routing::_isFile(const string &path)
{
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode));
}

string Routing::_getRoot(Location &loc)
{
    return (loc.root.empty() ? _server.root : loc.root);
}

size_t Routing::_getMaxBodySize(Location &loc)
{
    return (loc.maxBody ? loc.maxBody : _server.maxBody);
}

vector<string> Routing::_getIndexFiles(Location &loc)
{
    if (!loc.indexFiles.empty())
        return (loc.indexFiles);

    return (_server.indexFiles);
}
