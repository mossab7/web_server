#include "HTTPParser.hpp"
#include <iostream>
#include <cstdio>

HTTPParser::HTTPParser():
    _body(BUFF_SIZE * 16),
    _contentLength(0),
    _bytesRead(0),
    _isChunked(false),
    _chunkSize(0),
    _readChunkSize(0),
    _isMultiPart(false),
    _MultiParser(_body),
    _bodyHandler(NULL),
    _data(NULL),
    _isCGIResponse(false),
    _state(START_LINE),
    _buffOffset(0),
    _bodySize(0)
{
    _buffer.reserve(BUFF_SIZE);
}

std::string&    HTTPParser::getMethod(void) { return _method; }
std::string&    HTTPParser::getVers(void) { return _version; }
std::string&    HTTPParser::getUri(void) { return _uri; }
std::string&    HTTPParser::getQuery(void) { return _query; }
std::string&    HTTPParser::getFragment(void) { return _fragment; }

strmap&         HTTPParser::getHeaders(void) { return _headers; }
std::string&    HTTPParser::getHeader(const std::string& key) { return _headers[key]; }

RingBuffer&     HTTPParser::getBody(void) { return _body; }
size_t          HTTPParser::getBodySize(void) { return _bodySize; }
bool            HTTPParser::hasBody(void) { return _contentLength || _isChunked; }

parse_state     HTTPParser::getState(void) { return _state; }
bool    HTTPParser::isComplete(void)
{
    if (_isMultiPart)
        return _state == COMPLETE && _MultiParser.isComplete();
    return _state == COMPLETE;
}
bool    HTTPParser::isError(void)
{
    if (_isMultiPart)
        return _state == ERROR || _MultiParser.isError();
    return _state == ERROR;
}

void    HTTPParser::setCGIMode(bool m) { _isCGIResponse = m; }
bool    HTTPParser::getCGIMode(void) { return _isCGIResponse; }

void    HTTPParser::reset(void)
{
    _method.clear();
    _uri.clear();
    _version.clear();
    _body.clear();
    _headers.clear();
    
    if (_isCGIResponse)
        _state = HEADERS;
    else
        _state = START_LINE;
    _buffer.clear();
    _buffOffset = 0;

    _isChunked = false;
    _chunkSize = 0;
    _readChunkSize = 0;
 
    _contentLength = 0;
    _bytesRead = 0;

    _bodyHandler = NULL;
    _data = NULL;

    _isMultiPart = false;
    _boundary.clear();
    _MultiParser.reset();

    _bodySize = 0;
    //_isCGIResponse = false;
}

void    HTTPParser::addChunk(char* buff, size_t size)
{
    if (!buff || size <= 0)
        return;
    _buffer.append(buff, size);
    _parse();
}
void    HTTPParser::forceError() { _state = ERROR; }

void    HTTPParser::_parse()
{
label:
    parse_state old_state = _state;

    switch (_state)
    {
    case START_LINE:
        if (!_isCGIResponse)
        {
            _parseStartLine();
             break;
        }
    /* fall through */
    case HEADERS    : _parseHeaders(); break;
    case BODY       : _parseBody(); break;
    case CHUNK_SIZE : _parseChunkedSize(); break;
    case CHUNK_DATA : _parseChunkedSegment(); break;
    default: return;
    }

    if (_state == BODY)
        _bodySize = _contentLength;
    else if (_state == CHUNK_DATA)
        _bodySize += _chunkSize;

    if (_buffOffset * 2 >= BUFF_SIZE)
    {
        _buffer.erase(0, _buffOffset);
        _buffOffset = 0;
    }
    if (old_state != _state)
        goto label;
}       
void    HTTPParser::_parseStartLine()
{
    /*
    The Request-Line begins with a method token, followed by the
    Request-URI and the protocol version, and ending with CRLF. The
    elements are separated by SP characters. No CR or LF is allowed
    except in the final CRLF sequence.

        Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
    */
    if (_buffer.empty())
        return;
    size_t idx = _buffer.find(CRLF);
    if (idx == NPOS)
        return;
    _buffOffset = idx + 2;
    size_t i = 0;
    size_t sp = 0;
    for (size_t pos = 0; pos <= idx; ++pos)
    {
        if (_buffer[pos] != ' ' && pos != idx) continue;
        switch (sp)
        {
        case 0: _method = _buffer.substr(i, pos - i); break;
        case 1: _uri = _buffer.substr(i, pos - i); break;
        case 2: _version = _buffer.substr(i, pos - i); break;
        }
        if (pos != idx && ++sp > 2) break;
        i = pos + 1;
    }
    if (sp != 2 || _method.empty() || _uri.empty() || _version.empty())
        _state = ERROR;
    else
        _state = HEADERS;
    
    if (_state == ERROR)
        return;

    _decodeURI();
    if (_state == ERROR)
        return;
    size_t fragm = _uri.find('#');
    if (fragm != NPOS)
    {
        _fragment = _uri.substr(fragm + 1);
        _uri = _uri.erase(fragm);
    }
    size_t query = _uri.find('?');
    if (query != NPOS)
    {
        _query = _uri.substr(query + 1);
        _uri = _uri.substr(0, query);
    }
}
void    HTTPParser::_parseHeaders()
{
    /*
    message-header = field-name ":" [ field-value ]
        field-name     = token
        field-value    = *( field-content | LWS )
        field-content  = <the OCTETs making up the field-value
                         and consisting of either *TEXT or combinations
                         of token, separators, and quoted-string>
        NOTE: I don't feel like implementing the token,sperator quote stuff
    */
    if (_buffer.empty())
        return;

    while (true)
    {
        size_t idx = _buffer.find(CRLF, _buffOffset);
        if (idx == NPOS)
            return;
            
        if (idx == _buffOffset) // end of headers
        {
            _buffOffset += 2; // skip the empty line
            // Check if we need to parse body based on content length
            strmap::iterator it = _headers.find("content-length");
            strmap::iterator it2 = _headers.find("transfer-encoding");
            if (it2 != _headers.end())
            {
                std::string& enc = it2->second;
                std::transform(enc.begin(), enc.end(), enc.begin(), ::tolower);
                _state = (it2->second == "chunked" ? CHUNK_SIZE : ERROR);
                _isChunked = (_state == CHUNK_SIZE);
            }
            else if (it != _headers.end()) // prioritize chunked over con-lenth
            {
                char*   ptr = NULL;
                ssize_t len = std::strtol(it->second.data(), &ptr, 10);
                _state = (*ptr != '\0' || len < 0 ? ERROR : BODY);
                if (_state == ERROR) return;
                _contentLength = len;
                _state = (_contentLength == 0) ? COMPLETE : BODY;
            }
            else
                _state = _isCGIResponse ? BODY : COMPLETE;
            break;
        }

        // Find the first colon only
        size_t colon_pos = _buffer.find(':', _buffOffset);
        if (colon_pos == NPOS || colon_pos >= idx)
        {
            _state = ERROR;
            return;
        }

        std::string key = _buffer.substr(_buffOffset, colon_pos - _buffOffset);
        if (key.find_first_of(" \t") != NPOS)
        {
            _state = ERROR;
            return;
        }
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        std::string value = _buffer.substr(colon_pos + 1, idx - colon_pos - 1);
        
        // remove optional whitespace from value
        size_t value_start = value.find_first_not_of(" \t");
        size_t value_end = value.find_last_not_of(" \t");
        
        if (value_start != NPOS && value_end != NPOS)
            value = value.substr(value_start, value_end - value_start + 1);
        else
            value.clear(); // all whitespace

        _headers[key] = value;
        _buffOffset = idx + 2;
    }
    // why is this 
    std::string& cont_type = getHeader("content-type");
    if (cont_type.find("multipart/form-data") != NPOS)
    {
        _isMultiPart = true;
        size_t pos = cont_type.find("boundary=");
        _boundary = cont_type.substr(pos + 9);
        _MultiParser.setBoundry(_boundary);
    }
}
void    HTTPParser::_parseBody()
{
    if (_buffer.empty() || _buffOffset >= _buffer.size())
        return;
    
    size_t available = _buffer.size() - _buffOffset;
    if (_isCGIResponse)
    {
        _body.write(_buffer.data() + _buffOffset, available);
        _buffOffset += available;
        return;
    }

    size_t needed = _contentLength - _bytesRead;
    size_t to_read = std::min(available, needed);
    
    if (_bodyHandler)
        _bodyHandler(_buffer.data() + _buffOffset, to_read, _data);
    else
        _body.write(_buffer.data() + _buffOffset, to_read);
    _bytesRead += to_read;
    _buffOffset += to_read;
    
    if (_bytesRead >= _contentLength)
        _state = COMPLETE;
}

void    HTTPParser::_parseChunkedSize()
{
    if (_buffer.empty() || _buffOffset >= _buffer.size())
        return;

    size_t idx = _buffer.find(CRLF, _buffOffset);
    if (idx == NPOS)
        return;

    std::string chunkLine = _buffer.substr(_buffOffset, idx - _buffOffset);
    size_t extension_pos = chunkLine.find(';');
    if (extension_pos != NPOS)
        chunkLine = chunkLine.substr(0, extension_pos);

    try {
        _chunkSize = ft_atoi<size_t>(chunkLine);
    }
    catch (std::exception& e)
    {
        _state = ERROR;
        return;
    }

    _buffOffset = idx + 2;
    _readChunkSize = 0;
    
    if (_chunkSize == 0)
    {
        _state = COMPLETE;
        return;
    }
    
    _state = CHUNK_DATA;
}
void    HTTPParser::_parseChunkedSegment()
{
    if (_buffer.empty() || _buffOffset >= _buffer.size())
        return;
    size_t available = _buffer.size() - _buffOffset;
    size_t needed = _chunkSize - _readChunkSize;
    size_t to_read = std::min(available, needed);

    if (to_read > 0)
    {
        if (_bodyHandler)
            _bodyHandler(_buffer.data() + _buffOffset, to_read, _data);
        else
            _body.write(_buffer.data() + _buffOffset, to_read);
        _readChunkSize += to_read;
        _buffOffset += to_read;
    }

    if (_readChunkSize < _chunkSize)
        return;
    if (_buffer.size() - _buffOffset >= 2)
    {
        if (_buffer.substr(_buffOffset, 2) == CRLF)
        {
            _buffOffset += 2;
            _state = CHUNK_SIZE;
        }
        else
            _state = ERROR;
    }
}

void    HTTPParser::setBodyHandler(bodyHandler bh, void *data)
{
    _bodyHandler = bh;
    _data = data;
}
void    HTTPParser::setUploadDir(const std::string& dir) { _MultiParser.setUploadPath(dir); }

void    HTTPParser::_decodeURI()
{
    std::string tmp;
    long        nbr;

    for (size_t i = 0; i < _uri.size(); ++i)
    {
        if (_uri[i] != '%')
            continue;
        // '/%A' means a bad request
        if (i + 2 >= _uri.size())
        {
            _state = ERROR;
            return;
        }
        tmp = _uri.substr(i + 1, 2);
        nbr = std::strtol(tmp.c_str(), NULL, 16);
        if (!isascii(nbr))
        {
            _state = ERROR;
            return;
        }
        tmp = nbr;
        _uri.replace(i, 3, tmp);
    }
}

void    HTTPParser::parseMultipart()
{
    if (_isMultiPart)
        _MultiParser.parse();
}

bool    HTTPParser::isMultiPart() { return _isMultiPart; }