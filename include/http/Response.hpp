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

// helper macro to stringify values
#define SSTR(x) static_cast<std::ostringstream &>((std::ostringstream() << x)).str()

#define BUFF_SIZE 8192 // 8 KB buffer
#define CRLF "\r\n"

class HTTPResponse
{
    std::string _response; // headers + optional small body
    size_t _resp_offset;   // position in _response when sending

    int _file_fd;       // file descriptor (if serving file)
    size_t _file_size;  // total file size
    size_t _bytes_sent; // bytes sent from file

    // HTTPResponse(const HTTPResponse &other);
    // HTTPResponse &operator=(const HTTPResponse &other);

public:
    HTTPResponse(int code = 200, const std::string &status = "OK", const std::string &version = "HTTP/1.1");
    ~HTTPResponse(); // closes file if open

    // returns the class itself. todo shit like: res.add().add() ...
    HTTPResponse &addHeader(const std::string &name, const std::string &value);
    void endHeaders(); // marks end of headers

    // set body directly (for small responses)
    void setBody(const std::string &content);

    // serve file as body (sets Content-Length automatically)
    // this behavoir might change if we plan to support 'chunekd transfer'
    bool attachFile(const std::string &filepath);
    void closeFile();

    // write next chunk of data into buffer
    ssize_t readNextChunk(char *buffer, size_t buffer_size);

    // true if headers + file are fully sent
    bool isComplete() const;
};

HTTPResponse handleRequest(Routing &routing, const string &host, const string &request_path, const string &method);

#endif
