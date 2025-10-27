#ifndef WEBSERV_RESPONSE_HPP
#define WEBSERV_RESPONSE_HPP

#include <string>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <cstring>
#include <sstream>
#include "Routing.hpp"
#include "RingBuffer.hpp"

// helper macro to stringify values
#define SSTR(x) static_cast<std::ostringstream &>((std::ostringstream() << x)).str()

#define BUFF_SIZE 8192 // 8 KB buffer
#define CRLF "\r\n"

class HTTPResponse
{
    std::string _version;
    RingBuffer  _response;   // headers + optional small body

    int     _file_fd;       // file descriptor (if serving file)
    size_t  _file_size;     // total file size
    size_t  _bytes_sent;    // bytes sent from file
    
    
    const std::string _getContentType(const std::string &filepath);
    const std::string _getStatus(int code);
    public:
   // bool   _cgiComplete;
    HTTPResponse(const std::string& version);
    ~HTTPResponse();
    
    void    startLine(int code);

    void addHeader(const std::string &name, const std::string &value);
    void endHeaders();

    // set body directly (for small responses)
    // it automaticy sets content-length/type headers
    void setBody(const std::string& data, const std::string& type = "text/html");


    // used for chunked transfer (chunked transfer needs to be added in the headers)
    void feedRAW(const char* data, size_t size);
    void feedRAW(const std::string& data);

    // serve file as body (sets Content-Length automatically)
    // this behavoir might change if we plan to support 'chunekd transfer'
    bool attachFile(const std::string &filepath);
    void closeFile();

    // write next chunk of data into buffer
    ssize_t readNextChunk(char *buffer, size_t buffer_size);

    // true if headers + file are fully sent
    bool isComplete() const;

    void reset();
};

#endif
