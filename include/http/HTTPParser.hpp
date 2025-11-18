#ifndef WEBSERV_REQUEST_HPP
#define WEBSERV_REQUEST_HPP

#include <string>
#include <map>
#include <vector>
#include <cstring>
#include <algorithm>
#include <stdexcept>

#include "RingBuffer.hpp"
#include "multipart.hpp"

#define CRLF        "\r\n"
#define BUFF_SIZE   8192    // 8kb
#define NPOS        std::string::npos

typedef std::map<std::string, std::string> strmap;
typedef void (*bodyHandler)(const char* buff, size_t size, void *data);

enum parse_state
{
    START_LINE  ,
    HEADERS     ,
    BODY        ,       // simple body with content-length
    CHUNK_SIZE  ,       // transfer-encoding: chunked
    CHUNK_DATA  ,       ///
    COMPLETE    ,
    ERROR
};

#include <iostream>

template<typename t>
t ft_atoi(const std::string& n)
{
    char* ptr;
    t v = strtol(n.data(), &ptr, 16);
    if (*ptr)
        throw std::logic_error("invalid number");
    return v;
}

/*
    NOTE: from what i observed in nginx cgi response parsing:
    - https://raw.githubusercontent.com/nginx/nginx/refs/heads/master/src/http/modules/ngx_http_fastcgi_module.c
    - https://raw.githubusercontent.com/nginx/nginx/refs/heads/master/src/http/modules/ngx_http_scgi_module.c
    all i need to do to handle cgi is just set a flag to ignore the start line and move the headers directly :)


    another NOTE: use fallbacks to handle differenct types of requsts, aka pointer to function to handle:
        - normal http request
        - file uploads (write chunk directly to file)
        - cgi (write to the standart input of the process)
        - ... etc
*/

class HTTPParser
{
    // the requst head
    std::string _method;
    std::string _uri;
    std::string _query;
    std::string _fragment;
    std::string _version;
    strmap      _headers;

    // the requst body (default for now)
    RingBuffer  _body;
    size_t      _contentLength;
    size_t      _bytesRead;

    bool    _isChunked;
    size_t  _chunkSize;
    size_t  _readChunkSize; // the number of bytes read from the chunk

    bool        _isMultiPart;
    std::string _boundary;
    Multipart   _MultiParser;

    bodyHandler _bodyHandler;
    void        *_data;

    // cgi
    bool _isCGIResponse;

    // the current state
    parse_state _state;

    // the buffer holding the recived chunk
    std::string _buffer;
    size_t      _buffOffset;

    // this is the size of the request body
    // can be used with 'client_max_body_size'
    size_t  _bodySize;

    void    _decodeURI();
    void    _parseChunkedSize();
    void    _parseChunkedSegment();

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
    std::string&    getQuery(void);
    std::string&    getFragment(void);
    std::string&    getVers(void);

    strmap&         getHeaders(void);
    std::string&    getHeader(const std::string& key);

    void    setBodyHandler(bodyHandler bh, void *data);
    void    setUploadDir(const std::string& dir);
    
    void    setCGIMode(bool m);
    bool    getCGIMode(void);

    parse_state     getState();
    bool            isComplete();
    bool            isError();

    RingBuffer& getBody(void);
    bool        hasBody(void);
    size_t      getBodySize(void);

    void    reset();  // To reuse object for keep-alive connections

    void    addChunk(char* buff, size_t size); // feed next chunk to the object, parsed later
    void    parseMultipart();
    void    forceError();
};

#endif