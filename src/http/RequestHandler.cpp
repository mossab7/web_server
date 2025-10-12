#include "RequestHandler.hpp"

RequestHandler::RequestHandler(ServerConfig &config):
    _router(config)
{}
RequestHandler::~RequestHandler() { reset(); }

void    RequestHandler::feed(char* buff, size_t size) { _request.addChunk(buff, size); }

bool    RequestHandler::isReqComplete() { return _request.isComplete(); }
bool    RequestHandler::isResComplete() { return _response.isComplete(); }
bool    RequestHandler::isError() { return _request.isError(); }

size_t  RequestHandler::readNextChunk(char *buff, size_t size) { return _response.readNextChunk(buff, size); }

void    RequestHandler::reset()
{
    _request.reset();
    _response.reset();
}

bool    RequestHandler::keepAlive()
{
    std::string conn = _request.getHeader("connection");
    std::transform(conn.begin(), conn.end(), conn.begin(), ::tolower);

    if (_request.getVers() == "HTTP/1.1")
        return conn != "close";
    return conn == "keep-alive";
}

void    RequestHandler::processRequest()
{
    _keepAlive = keepAlive();
    // todo
    _response.startLine();
    _response.addHeader("Server", "WebServ/1.0");
    _response.addHeader("Connection", _keepAlive ? "keep-alive" : "close");
    _response.addHeader("content-length", "4");
    _response.addHeader("content-type", "text/html");
    _response.endHeaders();
    _response.setBody("todo");
    // RouteMatch match = _router.getMatch(_request.getUri(), _request.getMethod());
    // std::string path;
    // if (match.lc)
    // {
    //     if (_request.getUri() == "/")
    //         path = match.lc->root + '/' + match.lc->indexFiles[0];
    //     else
    //         path = match.lc->root + _request.getUri();
    // }
    // // Add logging to verify file attachment
    // if (!_response.attachFile(path)) // it already handles conent-len and content-type
    //     logger.error("Failed to attach file " + path + " on fd: ");
    //     // Fallback to error response
    //     // _buildErrorResponse(404, "File not found");
    // else
    //     logger.debug("Successfully attached file on fd: ");
}