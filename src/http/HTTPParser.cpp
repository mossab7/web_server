#include "HTTPParser.hpp"
#include <iostream>
#include <cstdio>

HTTPParser::HTTPParser():
    _contentLength(0),
    _bytesRead(0),
    _state(START_LINE),
    _buffOffset(0)
{
    _buffer.reserve(BUFF_SIZE);
    _body.reserve(BUFF_SIZE);
}

std::string&    HTTPParser::getMethod(void) {return _method; }
std::string&    HTTPParser::getVers(void) { return _version; }
std::string&    HTTPParser::getUri(void) { return _uri; }

strmap&         HTTPParser::getHeaders(void) { return _headers; }
std::string&    HTTPParser::getHeader(const std::string& key) { return _headers[key]; }

std::string&    HTTPParser::getBody(void) { return _body; }

parse_state     HTTPParser::getState(void) { return _state; }
bool    HTTPParser::isComplete(void) { return _state == COMPLETE; }
bool    HTTPParser::isError(void) { return _state == ERROR; }

void    HTTPParser::reset(void)
{
    _method.clear();
    _uri.clear();
    _version.clear();
    _body.clear();

    _headers.clear();

    _state = START_LINE;

    _buffer.clear();
    _buffOffset = 0;
    _contentLength = 0;
    _bytesRead = 0;
}

void    HTTPParser::addChunk(char* buff, size_t size)
{
    if (!buff || size <= 0)
        return;
    _buffer.append(buff, size);
    _parse();
}

void    HTTPParser::_parse()
{
    bool made_progress = true;
    
    while (made_progress && _state != COMPLETE && _state != ERROR)
    {
        parse_state old_state = _state;

        switch (_state)
        {
        case START_LINE: _parseStartLine(); break;
        case HEADERS: _parseHeaders(); break;
        case BODY: _parseBody(); break;
        case COMPLETE: case ERROR: break;
        }

        made_progress = (_state != old_state);
    }
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
            if (it != _headers.end())
            {
                char*   ptr = NULL;
                _contentLength = std::strtol(it->second.data(), &ptr, 10);
                _state = (*ptr ? ERROR : BODY);
                if (_state == ERROR) return;
            }
            else
                _state = COMPLETE;
            return;
        }

        // Find the first colon only
        size_t colon_pos = _buffer.find(':', _buffOffset);
        if (colon_pos == NPOS || colon_pos >= idx)
        {
            _state = ERROR;
            return;
        }

        std::string key = _buffer.substr(_buffOffset, colon_pos - _buffOffset);
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
}
void    HTTPParser::_parseBody()
{
    if (_buffer.empty() || _buffOffset >= _buffer.size())
        return;
        
    size_t available = _buffer.size() - _buffOffset;
    size_t needed = _contentLength - _bytesRead;
    size_t to_read = std::min(available, needed);
    
    _body.append(_buffer.begin() + _buffOffset, _buffer.begin() + _buffOffset + to_read);
    _bytesRead += to_read;
    _buffOffset += to_read;
    
    if (_bytesRead >= _contentLength)
        _state = COMPLETE;
}

