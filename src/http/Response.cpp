#include "Response.hpp"

HTTPResponse::HTTPResponse(const std::string& version):
    _version(version),
    _response(BUFF_SIZE * 2),
    _file_fd(-1),
    _file_size(0),
    _bytes_sent(0)
    //_cgiComplete(false)
{}

HTTPResponse::~HTTPResponse()
{
    closeFile();
}

void    HTTPResponse::addHeader(const std::string& k, const std::string& v)
{
    std::string line = k + ": " + v + CRLF;
    _response.write(line.data(), line.length());
}
void    HTTPResponse::endHeaders()
{
    _response.write(CRLF, 2);
}

void    HTTPResponse::setBody(const std::string& data, const std::string& type)
{
    addHeader("content-type", type);
    addHeader("content-length", SSTR(data.length()));
    endHeaders();
    _response.write(data.data(), data.length());
}

bool    HTTPResponse::attachFile(const std::string& filepath) {
    struct stat f;
    if (stat(filepath.c_str(), &f) != 0)
        return false;
    
    _file_fd = open(filepath.c_str(), O_RDONLY | O_NONBLOCK);
    if (_file_fd == -1)
        return false;

    _file_size = f.st_size;
    addHeader("Content-type", _getContentType(filepath));
    addHeader("Content-Length", SSTR(_file_size));
    endHeaders();
    
    return true;
}
void    HTTPResponse::closeFile()
{
    if (_file_fd != -1)
    {
        ::close(_file_fd);
        _file_fd = -1;
        _file_size = 0;
        _bytes_sent = 0;
    }
}

ssize_t HTTPResponse::readNextChunk(char* buff, size_t size)
{
    if (size == 0 || !buff) 
        return 0;

    // Send from the in-memory response buffer first
    size_t toSend = _response.read(buff, size);
    if (toSend)
        return toSend;

    // If the entire file has been sent, we're done
    if (_bytes_sent >= _file_size)
    {
        closeFile();
        return 0;
    }

    // read from the file
    ssize_t bytes = ::read(_file_fd, buff, size);
    if (bytes > 0)
        _bytes_sent += bytes;

    return bytes; // could be number of bytes read or -1 on error
}
#include "Logger.hpp"
bool    HTTPResponse::isComplete() const
{
    Logger logger;
    // if (_cgiComplete)
    //     return true;
    if (_response.getSize())
    {
        logger.debug("Response not complete: no response data");
        return false;
    }
    if (_file_size != _bytes_sent)
    {
        logger.debug("Response not complete: file size mismatch");
        return false;
    }
    return true;
}

void    HTTPResponse::reset()
{
    _response.clear();
    closeFile();
}

void    HTTPResponse::startLine(int code)
{
    std::string line = _version + ' ' + SSTR(code) + ' ' + _getStatus(code) + CRLF;
    _response.write(line.data(), line.length());
}

const std::string HTTPResponse::_getContentType(const std::string &filepath)
{
    size_t dotPos = filepath.rfind('.');
    if (dotPos == std::string::npos)
        return "application/octet-stream";
    
    const std::string& ext = filepath.substr(dotPos);
    
    // Text types
    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css")  return "text/css";
    if (ext == ".js")   return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".xml")  return "application/xml";
    if (ext == ".txt")  return "text/plain";
    
    // Image types
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".png")  return "image/png";
    if (ext == ".gif")  return "image/gif";
    if (ext == ".svg")  return "image/svg+xml";
    if (ext == ".ico")  return "image/x-icon";
    if (ext == ".webp") return "image/webp";
    
    // Font types
    if (ext == ".woff")  return "font/woff";
    if (ext == ".woff2") return "font/woff2";
    if (ext == ".ttf") return "font/ttf";
    if (ext == ".otf") return "font/otf";
    
    // Other
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".zip") return "application/zip";
    
    return "application/octet-stream";
}
const std::string HTTPResponse::_getStatus(int code)
{
    switch (code) {
        // 1xx: Informational
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        case 102: return "Processing";
        case 103: return "Early Hints";

        // 2xx: Success
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 203: return "Non-Authoritative Information";
        case 204: return "No Content";
        case 205: return "Reset Content";
        case 206: return "Partial Content";
        case 207: return "Multi-Status";
        case 208: return "Already Reported";
        case 226: return "IM Used";

        // 3xx: Redirection
        case 300: return "Multiple Choices";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 305: return "Use Proxy";
        // 306 is unused/reserved
        case 307: return "Temporary Redirect";
        case 308: return "Permanent Redirect";

        // 4xx: Client Error
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 402: return "Payment Required";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 407: return "Proxy Authentication Required";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 412: return "Precondition Failed";
        case 413: return "Payload Too Large";
        case 414: return "URI Too Long";
        case 415: return "Unsupported Media Type";
        case 416: return "Range Not Satisfiable";
        case 417: return "Expectation Failed";
        case 418: return "I'm a Teapot";          //  RFC 9110
        case 421: return "Misdirected Request";
        case 422: return "Unprocessable Entity";
        case 423: return "Locked";
        case 424: return "Failed Dependency";
        case 426: return "Upgrade Required";
        case 428: return "Precondition Required";
        case 429: return "Too Many Requests";
        case 431: return "Request Header Fields Too Large";
        case 451: return "Unavailable For Legal Reasons";

        // 5xx: Server Error
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        case 506: return "Variant Also Negotiates";
        case 507: return "Insufficient Storage";
        case 508: return "Loop Detected";
        case 510: return "Not Extended";
        case 511: return "Network Authentication Required";

        default:
            return "Unknown";
    }
}

void    HTTPResponse::feedRAW(const char* data, size_t size)
{
    std::stringstream ss;
    ss << std::hex << size;
    std::string sizeStr = ss.str();

    _response.write(sizeStr.data(), sizeStr.size());
    _response.write(CRLF, 2);

    _response.write(data, size);
    
    _response.write(CRLF, 2); 
}
void    HTTPResponse::feedRAW(const std::string& data)
{
    std::stringstream ss;
    ss << std::hex << data.size();
    std::string sizeStr = ss.str();

    _response.write(sizeStr.data(), sizeStr.size());
    _response.write(CRLF, 2);
    _response.write(data.data(), data.size());
    _response.write(CRLF, 2);
}

