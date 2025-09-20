#ifndef WEBSERV_REQUEST_HPP
#define WEBSERV_REQUEST_HPP

#include <string>
#include <map>
#include <vector>
#include <cstring>
#include <algorithm>

#define CRLF        "\r\n"
#define BUFF_SIZE   8192    // 8kb
#define NPOS        std::string::npos

typedef std::map<std::string, std::string> strmap;

enum parse_state
{
    START_LINE  ,
    HEADERS     ,
    BODY        ,       // POST
    COMPLETE    ,
    ERROR
};

class HTTPParser
{
    // the requst head
    std::string _method;
    std::string _uri;
    std::string _version;
    strmap      _headers;

    // the requst body (smaller ones)
    std::string _body;
    size_t      _contentLength;
    size_t      _bytesRead;

    // cgi and file upload stuff
    // std::string _filePath;
    // bool chunked transfer;

    // the current state
    parse_state _state;

    // the buffer holding the recived chunk
    std::string _buffer;
    size_t      _buffOffset;

    void    _parseBody();       // dependeing on 'Content-Type', the body is handled deferently
    void    _parseHeaders();
    void    _parseStartLine();
    void    _parse();

public:
    HTTPParser();

    // for the following funcitons, if an attribute like 'method' is not ready,
    // an empty string will be returned
    std::string&    getMethod(void);
    std::string&    getUri(void);
    std::string&    getVers(void);

    strmap&         getHeaders(void);
    std::string&    getHeader(const std::string& key);

    std::string&    getBody(void);

    parse_state     getState();
    bool            isComplete();
    bool            isError();

    void    reset();  // To reuse object for keep-alive connections

    void    addChunk(char* buff, size_t size); // feed next chunk to the object, parsed later
};

#endif