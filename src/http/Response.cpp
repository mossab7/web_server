#include "Response.hpp"

HTTPResponse::HTTPResponse(int code, const std::string& status, const std::string& version):
    _resp_offset(0),
    _file_fd(-1),
    _file_size(0),
    _bytes_sent(0)
{
    _response += version + ' ';
    _response += SSTR(code) + ' ';
    _response += status;
    _response += CRLF;
}

HTTPResponse::~HTTPResponse()
{
    closeFile();
}

HTTPResponse    &HTTPResponse::addHeader(const std::string& k, const std::string& v)
{
    if (k.empty() || v.empty()) // menas end of header
    {
        _response += CRLF;
        return *this;
    }

    _response += k + ": " + v + CRLF;

    return *this;
}

void    HTTPResponse::endHeaders()
{
    _response += CRLF;
}

void    HTTPResponse::setBody(const std::string& data)
{
    // ensure headers are properly terminated before adding body
    if (_response.size() && _response.rfind(CRLF CRLF) != _response.size() - 4)
        _response += CRLF;  // blank line between headers and body
  //  addHeader("Content-Length", SSTR(data.size()));
    //endHeaders();
    _response += data;
}

bool HTTPResponse::attachFile(const std::string& filepath) {
    struct stat f;
    if (stat(filepath.c_str(), &f) != 0)
        return false;
    
    _file_fd = open(filepath.c_str(), O_RDONLY | O_NONBLOCK);
    if (_file_fd == -1)
        return false;

    _file_size = f.st_size;
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
    }
}

ssize_t HTTPResponse::readNextChunk(char* buff, size_t size)
{
    if (size == 0 || !buff) 
        return 0;

    // Send from the in-memory response buffer first
    if (_resp_offset < _response.size())
    {
        size_t  left = _response.size() - _resp_offset;
        size_t  toCopy = std::min(size, left);
        std::memcpy(buff, _response.data() + _resp_offset, toCopy);
        _resp_offset += toCopy;

        if (_resp_offset >= _response.size())
        {
            _resp_offset = 0;
            _response.clear();
        }

        return toCopy;
    }

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
    if (_response.size())
        return false;
    if (_file_size != _bytes_sent)
        return false;
    return true;
}
