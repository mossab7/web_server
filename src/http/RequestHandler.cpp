#include "RequestHandler.hpp"

RequestHandler::RequestHandler(ServerConfig &config, HTTPParser& req, HTTPResponse& resp, FdManager &fdManager):
    _router(config),
    _request(req),
    _response(resp),
    _cgi(_request,_response,config,fdManager),
    _cgiSrtartTime(0),
    _keepAlive(false),
    _isCGI(false),
    _isDirSet(false),
    responseStarted(false)
{}
RequestHandler::~RequestHandler() 
{ 
    Logger logger;
    logger.debug("RequestHandler destructor called");
    //reset(); 
}

void    RequestHandler::feed(char* buff, size_t size) { _request.addChunk(buff, size); }

bool    RequestHandler::isReqComplete() { return _request.isComplete(); }
bool    RequestHandler::isReqHeaderComplete() { return _request.getState() > HEADERS; }
bool    RequestHandler::isResComplete() 
{ 
    // If CGI is running, response is not complete yet
    //logger.debug("Checking if response is complete");
     if (_isCGI && _cgi.isRunning())
     {
         //logger.debug("Response not complete: CGI still running");
         return false;
     }
    return _response.isComplete(); 
}
bool    RequestHandler::isError() { return _request.isError(); }

size_t  RequestHandler::readNextChunk(char *buff, size_t size)
{
    const RouteMatch& match = _router.match(_request.getUri(), _request.getMethod());
	//logger.debug("cgi timeout: " + intToString(match.location->cgi_timeout));
    //logger.debug("time diff: " + intToString(static_cast<int>(difftime(time(NULL), _cgiSrtartTime))));
    //logger.debug("time now: " + intToString(static_cast<int>(time(NULL))));
    //logger.debug("cgi start time: " + intToString(static_cast<int>(_cgiSrtartTime)));
    // if (_isCGI && difftime(time(NULL), _cgiSrtartTime) >= match.location->cgi_timeout)
	// {
    //     _cgi.end();
    //     logger.error("CGI timeout reached for script: " + match.scriptPath);
    //     logger.debug("responseStarted: " + std::string(responseStarted ? "true" : "false"));
    //     if (responseStarted == false)
    //     {
    //         _sendErrorResponse(504);
    //     }
    //     else 
	// 	    return (-1);
	// }
    if (_isCGI && _cgi.getStatus() != 0)
    {
        if (responseStarted == false)
            _sendErrorResponse(_cgi.getStatus());
        else 
            return (-1);
    }
    return _response.readNextChunk(buff, size);
}

void    RequestHandler::reset()
{
    logger.warning("Resetting RequestHandler state");
    _isCGI = false;
    _request.reset();
    _response.reset();
    _isDirSet = false;
    _cgi.reset();
}

bool    RequestHandler::keepAlive()
{
    std::string conn = _request.getHeader("connection");
    std::transform(conn.begin(), conn.end(), conn.begin(), ::tolower);

    if (_request.getVers() == "HTTP/1.1")
        return conn != "close";
    return conn == "keep-alive";
}

bool    RequestHandler::processRequest()
{
    _keepAlive = keepAlive();

    const RouteMatch& match = _router.match(_request.getUri(), _request.getMethod());
    
    if (!match.isValidMatch())
    {
        logger.error("Not a valid match: " + _request.getUri());
        _sendErrorResponse(404);
        return true;
    }
    if (!match.methodAllowed)
    {
        logger.error("Method not allowed: " + _request.getMethod());
        _sendErrorResponse(405);
        return true;
    }

    _isCGI = match.isCGI;
    const std::string& method = _request.getMethod();
    logger.debug("rquest method : " + method);
    if (method == "GET")
        _handleGET(match);
    else if (method == "POST")
        _handlePOST(match);
    else if (method == "DELETE")
        _handleDELETE(match);
    else
        _sendErrorResponse(501);

    return _request.isComplete();
}

void    RequestHandler::_common(const RouteMatch& match)
{
    // 1. Check redirects
    // 2. Check if path exists (404 if not)
    // 3. If directory: call _servDict
    // 4. If file: serve it

    if (match.isRedirect)
    {
        _response.startLine(301);
        _response.addHeader("location", match.redirectUrl);
        _response.addHeader("Cache-Control", "no-cache");
        _response.endHeaders();
        return;
    }
    if (!match.doesExist)
    {
        logger.error("match does not exist: " + match.fsPath);
        _sendErrorResponse(404);
        return;
    }
    if (match.isDirectory)
    {
        const std::string& uri = _request.getUri();
        if (uri.empty() || uri[uri.length() - 1] != '/')
        {
            std::string redirectUri = uri + '/';
            _response.startLine(301);
            _response.addHeader("location", redirectUri);
            _response.addHeader("content-length", "0");
            _response.endHeaders();
            return;
        }
        _serveDict(match);
        return;
    }
    if (match.isFile)
    {
        _serveFile(match);
        return;
    }
}

void    RequestHandler::_handleGET(const RouteMatch& match)
{
    // 1. just does common stuff like file or dict serving
    if (_isCGI)
        _handleCGI(match);
    else
    {
        logger.debug("handle GET common is called");
        _common(match);
    }
}
void    RequestHandler::_handlePOST(const RouteMatch& match)
{

    // 1. Check if upload is allowed (match.isUploadAllowed())
    // 2. Check body size against maxBodySize (413 if too large)
    // 3. If CGI: handle via CGI
    // 4. If upload: save file to uploadDir
    // 5. Return 201 Created or appropriate response

    if (_request.getBodySize() > match.maxBodySize)
    {
        logger.error("max body size reached");
        _sendErrorResponse(413);
        _request.forceError();
        return;
    }
    if (match.isUploadAllowed())
    {
        if (!_isDirSet)
        {
            _request.setUploadDir(match.uploadDir);
            _isDirSet = true;
        }
        _request.parseMultipart();
        if (_request.isError())
        {
            _sendErrorResponse(500);
            return;
        }
        if (_request.isComplete())
        {
            _response.startLine(201);
            _response.addHeader("content-length", "0");
            _response.endHeaders();
        }
        return;
    }
    if (_isCGI)
        _handleCGI(match);
    else
        _common(match);
}
void    RequestHandler::_handleDELETE(const RouteMatch& match)
{
    // 1. Check if path exists (404 if not)
    // 2. Check if it's a file (403 if directory)
    // 3. Check permissions
    // 4. Delete the file with unlink()
    // 5. Return 204 No Content or 200 OK
    if (!match.doesExist)
    {
        _sendErrorResponse(404);
        return;
    }
    if (match.isDirectory)
    {
        _sendErrorResponse(403);
        return;
    }
    if (unlink(match.fsPath.c_str()) == 0)
    {
        logger.success("file wad deleted: " + match.fsPath);
        _response.startLine(204);
        _response.endHeaders();
        return;
    }
    else
        _sendErrorResponse(403);
}

void    RequestHandler::_sendErrorResponse(int code)
{
    _response.reset();
    _response.startLine(code);
    if (!_response.attachFile(_router.getErrorPage(code)))
        _response.setBody(getErrorPage(code));
}

void    RequestHandler::_serveFile(const RouteMatch& path)
{
    _response.startLine(200);
    if (!_response.attachFile(path.fsPath))
    {
        logger.error("cant send file: " + path.fsPath);
        _sendErrorResponse(403);
    }
}
void    RequestHandler::_serveDict(const RouteMatch& path)
{
    //    - Check autoindex
    //    - Try index files
    //    - 403 if neither
    _response.startLine(200);
    if (path.autoIndex)
    {
        _response.setBody(_getDictListing(path.fsPath));
        return;
    }
    for (size_t i = 0; i < path.indexFiles.size(); ++i)
    {
        if (_response.attachFile(path.fsPath + '/' + path.indexFiles[i]))
            return;
    }
    _sendErrorResponse(403);
}

std::string RequestHandler::_getDictListing(const std::string& path)
{
    DIR* dir = opendir(path.c_str());
    if (!dir)
    {
        logger.error("Failed to open directory: " + path);
        return "";
    }

    std::string reqUri = _request.getUri();

    // final html code
    std::string html;
    html.reserve(BUFF_SIZE * 2);

    html += "<!DOCTYPE html>\n";
    html += "<html lang=\"en\">\n<head>\n";
    html += "<meta charset=\"UTF-8\">\n";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html += "<title>Index of " + reqUri + "</title>\n";
    html += "<style>\n";
    html += "* { margin: 0; padding: 0; box-sizing: border-box; }\n";
    html += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif; background: #f5f5f5; min-height: 100vh; padding: 40px 20px; }\n";
    html += ".container { max-width: 900px; margin: 0 auto; }\n";
    html += "header { background: white; border-radius: 4px; padding: 30px; box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1); margin-bottom: 32px; }\n";
    html += ".status { display: inline-flex; align-items: center; padding: 8px 16px; background: #f0fdf4; color: #166534; border: 1px solid #bbf7d0; border-radius: 3px; font-size: 12px; font-weight: 500; margin-bottom: 20px; text-transform: uppercase; letter-spacing: 0.05em; }\n";
    html += ".status::before { content: ''; width: 8px; height: 8px; background: #22c55e; border-radius: 50%; margin-right: 8px; animation: pulse 2s ease-in-out infinite; }\n";
    html += "@keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.5; } }\n";
    html += "h1 { color: #1a1a1a; font-size: 32px; font-weight: 700; letter-spacing: -0.02em; margin-bottom: 8px; }\n";
    html += ".subtitle { color: #737373; font-size: 14px; font-family: 'SF Mono', Monaco, 'Courier New', monospace; }\n";
    html += ".section { background: white; border-radius: 4px; padding: 32px; box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1); }\n";
    html += ".section-title { font-size: 13px; font-weight: 600; text-transform: uppercase; letter-spacing: 0.05em; color: #404040; margin-bottom: 1px; padding-bottom: 12px; border-bottom: 1px solid #e5e5e5; }\n";
    html += "table { width: 100%; border-collapse: collapse; }\n";
    html += "thead th { text-align: left; padding: 12px 16px; font-size: 12px; font-weight: 600; text-transform: uppercase; letter-spacing: 0.05em; color: #737373; border-bottom: 1px solid #e5e5e5; }\n";
    html += "tbody tr { border-bottom: 1px solid #f5f5f5; transition: background-color 0.2s; }\n";
    html += "tbody tr:hover { background-color: #fafafa; }\n";
    html += "tbody td { padding: 16px; font-size: 14px; color: #1a1a1a; }\n";
    html += "tbody td.name { display: flex; align-items: center; gap: 8px; }\n";
    html += "tbody td.name a { font-family: 'SF Mono', Monaco, 'Courier New', monospace; }\n";
    html += "tbody td.size, tbody td.date { color: #737373; font-size: 13px; }\n";
    html += "a { text-decoration: none; color: #1a1a1a; }\n";
    html += "a:hover { color: #0066cc; }\n";
    html += ".badge { font-size: 11px; padding: 4px 8px; border-radius: 2px; font-weight: 500; text-transform: uppercase; letter-spacing: 0.03em; display: inline-block; }\n";
    html += ".badge-dir { background: #fef3c7; color: #92400e; border: 1px solid #fde68a; }\n";
    html += ".badge-file { background: #f0f9ff; color: #0369a1; border: 1px solid #bae6fd; }\n";
    html += ".info-box { background: #fafafa; border: 1px solid #e5e5e5; border-radius: 3px; padding: 20px; margin-top: 24px; }\n";
    html += ".info-title { font-weight: 600; margin-bottom: 8px; font-size: 13px; text-transform: uppercase; letter-spacing: 0.05em; color: #404040; }\n";
    html += ".server-info { font-family: 'SF Mono', Monaco, 'Courier New', monospace; font-size: 13px; color: #737373; }\n";
    html += "</style>\n";
    html += "</head>\n<body>\n";
    html += "<div class=\"container\">\n";
    html += "<header>\n";
    html += "<div class=\"status\">Directory Listing</div>\n";
    html += "<h1>Index of " + reqUri + "</h1>\n";
    html += "<div class=\"subtitle\">WebSrv 1.0</div>\n";
    html += "<div class=\"info-box\">\n";
    html += "<div class=\"info-title\">System Path</div>\n";
    html += "<div class=\"server-info\">" + path + "</div>\n";
    html += "</div>\n";
    html += "</header>\n";
    html += "<div class=\"section\">\n";
    html += "<div class=\"section-title\">Contents</div>\n";
    html += "<table>\n";
    html += "<thead>\n<tr>\n";
    html += "<th>Name</th>\n";
    html += "<th>Size</th>\n";
    html += "<th>Last Modified</th>\n";
    html += "</tr>\n</thead>\n<tbody>\n";

    // this will be used to store entries data
    std::vector<fileInfo> entries;

    // collect entries
    dirent *entry = NULL;
    while ((entry = readdir(dir)))
    {
        fileInfo info;
        info.name = entry->d_name;
        if (info.name == ".")
            continue;
        std::string epath = path + '/' + info.name;
        if (stat(epath.c_str(), &info.data) == 0)
            entries.push_back(info);
    }
    closedir(dir);

    // sort this shit, dicts first, then alphabitcly
    std::sort(entries.begin(), entries.end());

    // build the html
    for (size_t i = 0; i < entries.size(); ++i)
    {
        bool isDir = entries[i].data.st_mode & S_ISDIR(entries[i].data.st_mode);
        std::string link = reqUri + entries[i].name;

        std::string sizeStr = "-";
        std::string dateStr = "-";

        // format size
        if (!isDir)
        {
            size_t fileSize = entries[i].data.st_size;
            if (fileSize < 1024)
                sizeStr = SSTR(fileSize) + " B";
            else if (fileSize < 1024 * 1024)
                sizeStr = SSTR(fileSize / 1024) + " KB";
            else if (fileSize < 1024 * 1024 * 1024)
                sizeStr = SSTR(fileSize / (1024 * 1024)) + " MB";
            else
                sizeStr = SSTR(fileSize / (1024 * 1024 * 1024)) + " GB";
        }

        // format time
        char timeBuff[100];
        struct tm *time = std::localtime(&entries[i].data.st_mtime);
        if (time)
        {
            std::strftime(timeBuff, sizeof(timeBuff), "%Y-%m-%d %H:%M", time);
            dateStr = timeBuff;
        }

        html += "<tr>\n";
        html += "<td class=\"name\">";
        html += "<span class=\"badge badge-" + std::string(isDir ? "dir" : "file") + "\">" +
                std::string(isDir ? "DIR" : "FILE") + "</span>";
        html += "<a href=\"" + link + "\">" + entries[i].name +
                (isDir ? "/" : "") + "</a></td>\n";
        html += "<td class=\"size\">" + sizeStr + "</td>\n";
        html += "<td class=\"date\">" + dateStr + "</td>\n";
        html += "</tr>\n";
    }

    html += "</table>\n</div>\n</div>\n</body>\n</html>";

    return html;
}

void    RequestHandler::_handleCGI(const RouteMatch& match)
{
    // idk pas the response to fill it or smth
    // run the script, see RouteMatch for more info.. etc
    logger.debug("cgi start is called");
    _cgiSrtartTime = time(NULL);
    _cgi.start(match, _request.hasBody());
}

void    RequestHandler::setError(int code) { _sendErrorResponse(code); }
