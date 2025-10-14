#include "Response.hpp"

HTTPResponse::HTTPResponse():
    _response(BUFF_SIZE * 2),
    _file_fd(-1),
    _file_size(0),
    _bytes_sent(0)
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

void    HTTPResponse::setBody(const std::string& data)
{
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

bool    HTTPResponse::isComplete() const
{
    if (!_response.getSize())
        return false;
    if (_file_size != _bytes_sent)
        return false;
    return true;
}

void    HTTPResponse::reset()
{
    _response.clear();
    closeFile();
}

void    HTTPResponse::startLine(int code, const std::string &status, const std::string &version)
{
    std::string line = version + ' ' + SSTR(code) + ' ' + status + CRLF;
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
