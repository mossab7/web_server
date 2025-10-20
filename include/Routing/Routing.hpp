#ifndef ROUTING_HPP
#define ROUTING_HPP

#include "ConfigParser.hpp"
#include "HTTPParser.hpp"
#include <sys/stat.h>

struct RouteMatch
{
    Location *location;
    bool isMatched;
    bool methodAllowed;

    std::string fsPath;
    std::string scriptPath;
    std::string scriptInterpreter;
    std::string pathInfo;

    bool isCGI;
    bool isRedirect;
    bool isDirectory;
    bool isFile;
    bool doesExist;

    bool autoIndex;
    std::string uploadDir;
    std::string redirectUrl;
    size_t maxBodySize;
    std::vector<std::string> indexFiles;
    RouteMatch();

    bool isValidMatch() const;
    bool isUploadAllowed() const;
};

class Routing
{
private:
    ServerConfig &_server;

    Location *_findLocation(const std::string &path);
    bool _matchesRoute(const std::string &path, const std::string &route);

    std::string _resolvePath(Location &loc, const std::string &reqPath);
    std::string _cleanPath(const std::string &path);
    std::string _joinPath(const std::string &base, const std::string &path);
    std::string _getRelativePath(const std::string &path, const std::string &route);

    bool _isCGI(Location &loc);
    void _splitCGIPath(const std::string &fsPath, std::string &scriptPath, std::string &pathInfo);

    bool _isMethodAllowed(Location &loc, const std::string &method);

    bool _isPathExists(const std::string &path);
    bool _isDirectory(const std::string &path);
    bool _isFile(const std::string &path);

    std::string _getRoot(Location &loc);
    size_t _getMaxBodySize(Location &loc);
    std::vector<std::string> _getIndexFiles(Location &loc);

public:
    Routing(ServerConfig &server);

    RouteMatch match(const std::string &path, const std::string &method);
    std::string getErrorPage(int code);
    std::string getAllowedMethodsStr(Location &loc);
};

#endif
