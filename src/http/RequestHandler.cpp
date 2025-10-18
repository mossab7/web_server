#include "RequestHandler.hpp"

RequestHandler::RequestHandler(ServerConfig &config, HTTPParser& req, HTTPResponse& resp):
    _router(config),
    _request(req),
    _response(resp)
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
    
    const RouteMatch& match = _router.match(_request.getUri(), _request.getMethod());
    if (!match.isValidMatch())
    {
        logger.error("Not a valid match: " + _request.getUri());
        _sendErrorResponse(404);
        return;
    }

    const std::string& method = _request.getMethod();
    if (method == "GET")
        _handleGET(match);
    else if (method == "POST")
        _handlePOST(match);
    else if (method == "DELETE")
        _handleDELETE(match);
    else
        _sendErrorResponse(501);
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
    _common(match);
    // 1. just does common stuff like file or dict serving
}
void    RequestHandler::_handlePOST(const RouteMatch& match)
{
    _common(match);
    // 1. Check if upload is allowed (match.isUploadAllowed())
    // 2. Check body size against maxBodySize (413 if too large)
    // 3. If CGI: handle via CGI
    // 4. If upload: save file to uploadDir
    // 5. Return 201 Created or appropriate response
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
    html += "<!DOCTYPE html>\n";
    html += "<html>\n<head>\n";
    html += "<meta charset=\"utf-8\">\n";
    html += "<title>Index of " + reqUri + "</title>\n";
    html += "<style>\n";
    html += "body { font-family: monospace; margin: 40px; }\n";
    html += "h1 { border-bottom: 1px solid #ccc; padding-bottom: 10px; }\n";
    html += "table { border-collapse: collapse; width: 100%; }\n";
    html += "th { text-align: left; padding: 8px; border-bottom: 2px solid #ddd; }\n";
    html += "td { padding: 8px; border-bottom: 1px solid #eee; }\n";
    html += "tr:hover { background-color: #f5f5f5; }\n";
    html += "a { text-decoration: none; color: #0066cc; }\n";
    html += "a:hover { text-decoration: underline; }\n";
    html += ".dir::before { content: '📁 '; }\n";
    html += ".file::before { content: '📄 '; }\n";
    html += ".info { color: #666; }\n";
    html += "</style>\n";
    html += "</head>\n<body>\n";
    html += "<h3>WebSrv 1.0</h3>\n";
    html += "<h1>Index of " + reqUri + "</h1>\n";
    html += "<table>\n";
    html += "<thead>\n<tr>\n";
    html += "<th>Name</th>\n";
    html += "<th class=\"info\">Size</th>\n";
    html += "<th class=\"info\">Last Modified</th>\n";
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
                sizeStr = SSTR(fileSize) + "B";
            else if (fileSize < 1024 * 1024)
                sizeStr = SSTR(fileSize / 1024) + "KB";
            else if (fileSize < 1024 * 1024 * 1024)
                sizeStr = SSTR(fileSize / (1024 * 1024)) + "MB";
            else
                sizeStr = SSTR(fileSize / (1024 * 1024 * 1024)) + "GB";
        }

        // format time
        char timeBuff[100];
        struct tm *time = std::localtime(&entries[i].data.st_mtime);
        if (time)
        {
            std::strftime(timeBuff, sizeof(timeBuff), "%Y-%m-%d %H:%M:%S", time);
            dateStr = timeBuff;
        }

        html += "<tr>\n";
        html += "<td><a href=\"" + link + "\" class=\"" + 
            (isDir ? "dir" : "file") + "\">" + entries[i].name + 
            (isDir ? "/" : "") + "</a></td>\n";
        html += "<td class=\"size\">" + sizeStr + "</td>\n";
        html += "<td class=\"date\">" + dateStr + "</td>\n";
        html += "</tr>\n";
    }

    html += "</tbody>\n</table>\n";
    html += "</body>\n</html>";

    return html;
}