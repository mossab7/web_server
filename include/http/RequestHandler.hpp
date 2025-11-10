#ifndef WEBSERV_REQ_HANDLER
#define WEBSERV_REQ_HANDLER

#include <dirent.h>

#include "HTTPParser.hpp"
#include "Response.hpp"
#include "Routing.hpp"
#include "Logger.hpp"
#include "SpecialResponse.hpp"
#include "../cgi/CGIHandler.hpp"

// the current methods we are required to handle
// static const char* methods[] = { "GET", "POST", "DELETE" };

class RequestHandler
{
    Logger  logger;

    Routing         _router;
    HTTPParser      &_request;
    HTTPResponse    &_response;

    CGIHandler      _cgi;
	time_t			_cgiSrtartTime;

    bool            _keepAlive;
    bool            _isCGI;
    bool            _isDirSet;

    void    _common(const RouteMatch& match);
    // i wanted to use an iteface for this, but it's overkill
    void    _handleGET(const RouteMatch& match);
    void    _handlePOST(const RouteMatch& match);
    void    _handleDELETE(const RouteMatch& match);
    void    _handleOPTIONS();

    // helper methods
    void        _sendErrorResponse(int code);
    void        _serveFile(const RouteMatch& path);
    void        _serveDict(const RouteMatch& match);
    std::string _getDictListing(const std::string& path);

    void        _handleCGI(const RouteMatch& match);

    struct fileInfo
    {
        std::string name;
        struct stat data;
        bool operator<(const fileInfo& other) const
        {
            bool thisIsDir = S_ISDIR(data.st_mode);
            bool otherIsDir = S_ISDIR(other.data.st_mode);

            if (thisIsDir != otherIsDir)
                return thisIsDir > otherIsDir;

            return name < other.name;
        }
    };

public:
    RequestHandler(ServerConfig &config, HTTPParser& req, HTTPResponse& resp, FdManager &fdManager);
    ~RequestHandler();

    void    feed(char* buff, size_t size);

    bool    isReqComplete();
    bool    isReqHeaderComplete();

    bool    isResComplete();
    bool    isError();
    bool    keepAlive();
    bool    responseStarted;

    void    setError(int code);

    // return true if response is ready to be sent
    bool    processRequest();
    size_t  readNextChunk(char* buff, size_t size);

    void    reset();
};


#endif
