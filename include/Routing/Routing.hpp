#ifndef ROUTING_HPP
#define ROUTING_HPP

#include "ConfigParser.hpp"
#include <string>
#include <vector>
#include <sys/stat.h>

/**
 * @brief Result of route matching with resolved paths and settings
 */
struct RouteMatch
{
    Location        *location;      // Matched location block
    bool            valid;          // Valid match found?
    bool            methodAllowed;  // HTTP method allowed?
    
    // Resolved paths
    std::string     fsPath;         // Full filesystem path
    std::string     scriptPath;     // CGI script path (if isCGI)
    std::string     pathInfo;       // PATH_INFO for CGI
    
    // Flags
    bool            isCGI;          // Handle with CGI?
    bool            isRedirect;     // Redirect requested?
    bool            isDirectory;    // Target is directory?
    
    // Cached settings
    bool            autoIndex;      
    std::string     uploadDir;      
    std::string     redirectUrl;    
    size_t          maxBodySize;    
    std::vector<std::string> indexFiles;
    
    RouteMatch() : location(NULL), valid(false), methodAllowed(false),
                   isCGI(false), isRedirect(false), isDirectory(false),
                   autoIndex(false), maxBodySize(0) {}
    
    bool isValid() const { return valid && location != NULL; }
    bool canUpload() const { return !uploadDir.empty(); }
};

/**
 * @brief Simple router for path matching and resolution
 * Assumes ServerConfig is already known
 */
class Routing
{
private:
    ServerConfig    &_server;   // Reference to server config
    
    // === Location Matching ===
    
    /**
     * @brief Find best matching location using longest prefix match
     * @param path Request URI path
     * @return Best matching location or NULL
     */
    Location*       _findLocation(const std::string &path);
    
    /**
     * @brief Check if path matches location route
     * @param path Request path
     * @param route Location route prefix
     * @return True if path starts with route
     */
    bool            _matchesRoute(const std::string &path, const std::string &route);
    
    // === Path Resolution ===
    
    /**
     * @brief Resolve request path to filesystem path
     * @param loc Location config
     * @param reqPath Request URI path
     * @return Full filesystem path
     * 
     * Logic:
     * 1. Get root (location.root or server.root)
     * 2. Remove location prefix from request path
     * 3. Join root + remaining path
     * 4. Normalize result
     */
    std::string     _resolvePath(Location &loc, const std::string &reqPath);
    
    /**
     * @brief Normalize path (remove .., ., multiple slashes)
     * @param path Input path
     * @return Normalized path
     */
    std::string     _normalizePath(const std::string &path);
    
    /**
     * @brief Join two paths safely
     */
    std::string     _joinPath(const std::string &base, const std::string &path);
    
    /**
     * @brief Remove location prefix from request path
     * Example: "/api/users" with prefix "/api" → "/users"
     */
    std::string     _stripPrefix(const std::string &path, const std::string &prefix);
    
    // === CGI Handling ===
    
    /**
     * @brief Check if this is a CGI request
     * @param loc Location config
     * @return True if location has CGI configured
     */
    bool            _isCGI(Location &loc);
    
    /**
     * @brief Split filesystem path into script and PATH_INFO
     * @param fsPath Full path like "/var/www/script.php/extra/path"
     * @param scriptPath Output: "/var/www/script.php"
     * @param pathInfo Output: "/extra/path"
     * 
     * Finds first existing file component, rest becomes PATH_INFO
     */
    void            _splitCGIPath(const std::string &fsPath,
                                  std::string &scriptPath,
                                  std::string &pathInfo);
    
    // === Method Validation ===
    
    /**
     * @brief Check if method is allowed in location
     * @param loc Location config
     * @param method HTTP method string
     * @return True if allowed (or if no methods specified = allow all)
     */
    bool            _isMethodAllowed(Location &loc, const std::string &method);
    
    // === Filesystem Checks ===
    
    bool            _exists(const std::string &path);
    bool            _isDirectory(const std::string &path);
    bool            _isFile(const std::string &path);
    
    // === Config Helpers ===
    
    /**
     * @brief Get effective root (location.root or fallback to server.root)
     */
    std::string     _getRoot(Location &loc);
    
    /**
     * @brief Get effective max body size
     */
    size_t          _getMaxBodySize(Location &loc);
    
    /**
     * @brief Get effective index files
     */
    std::vector<std::string> _getIndexFiles(Location &loc);

public:
    /**
     * @brief Constructor with server config reference
     * @param server Server configuration to use
     */
    Routing(ServerConfig &server);
    
    /**
     * @brief Main routing function
     * @param path Request URI path
     * @param method HTTP method
     * @return Complete route match with all info needed
     */
    RouteMatch  match(const std::string &path, const std::string &method);
    
    // === Utilities ===
    
    /**
     * @brief Get custom error page path for status code
     * @param code Status code
     * @return Path to error page or empty string
     */
    std::string getErrorPage(int code);
    
    /**
     * @brief Get formatted string of allowed methods for location
     * @param loc Location
     * @return e.g., "GET, POST, DELETE"
     */
    std::string getAllowedMethodsStr(Location &loc);
};

#endif // WEBSERV_ROUTING_HPP
