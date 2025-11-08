#include "multipart.hpp"
#include <iostream>
#include <cstdio>

Multipart::Multipart(RingBuffer& body):
    _state(ST_SEEKBOUND),
    _buff(body)
{ }
Multipart::~Multipart()
{
    if (_outfile.is_open())
        _outfile.close();
}


void    Multipart::setUploadPath(const std::string& path) { _uploadDict = path; }
void    Multipart::setBoundry(const std::string& bound)
{
    _str_boundry  = "--" + bound;
    if (_str_boundry.size() >= sizeof(_tmp_buff))
    {
        _logger.error("Boundry too large 'RFC 2046'");
        _onError();
    }
}

Multipart::parts_t&   Multipart::getParts(void) { return _parts; }
multipartState      Multipart::getState(void) { return _state; }

bool    Multipart::isComplete(void) { return _state == ST_COMPLETE; }
bool    Multipart::isError(void) { return _state == ST_MERROR; }

// the body is something like this:
/*
--------------------------840a7da8fa23b607
Content-Disposition: form-data; name="file"; filename="photo.jpg"
Content-Type: image/jpeg

[BINARY DATA]
--------------------------840a7da8fa23b607
Content-Disposition: form-data; name="bio"

this is just a dumy text. i think normal texts like this should be 
outputed to the console no need to store them anywhere
--------------------------840a7da8fa23b607
Content-Disposition: form-data; name="body"; filename="photo.jpg"
Content-Type: image/jpeg

[BINARY DATA]
--------------------------840a7da8fa23b607--
*/
void    Multipart::parse()
{
label:
    multipartState old = _state;

    switch (_state)
    {
    case ST_SEEKBOUND   : _seekBound(); break;
    case ST_HEADERS     : _parseHeaders(); break;
    case ST_DATA        : _parseData(); break;
    case ST_SAVEPART    : _savePart(); break;
    case ST_MERROR      : return;
    case ST_COMPLETE    : return;
    }
    if (_state != old)
        goto label;   // this is my first time using goto. it's kinda cool
}

void    Multipart::_seekBound()
{
    size_t s = _buff.peek(_tmp_buff, sizeof(_tmp_buff) - 1);
    if (!s) return;

    _tmp_buff[s] = 0;

    char *str = std::strstr(_tmp_buff, _str_boundry.data());
    if (!str) // advace the read 
    {
        if (s > _str_boundry.size())
            _buff.advanceRead(s - _str_boundry.size());
        return;
    }

    // this equation came from an example like:
    // "     ---------testfoo" CRLF
    _buff.advanceRead(_str_boundry.size() + (str - _tmp_buff));
    _state = ST_HEADERS;

    s = _buff.peek(_tmp_buff, 2); // i just want to check the end bound
    if (s >= 2 && !std::strncmp(_tmp_buff, "--", 2))
    {
        _state = ST_COMPLETE;
        _buff.clear();
    }
}
void    Multipart::_parseHeaders()
{
    size_t s = _buff.peek(_tmp_buff, sizeof(_tmp_buff));
    if (!s) return;

    _tmp_buff[s] = 0;
    static const char *bound = CRLF CRLF"\0";
    char *str = std::strstr(_tmp_buff, bound);
    if (!str) return;

    _buff.advanceRead(std::strlen(bound) + (str - _tmp_buff));
    _state = ST_DATA;

    // we take only the headers
    std::string tmp(_tmp_buff, str - _tmp_buff);

    size_t pos = tmp.find("name=\"");
    size_t end = tmp.find("\"", pos + 6);

    if (pos == std::string::npos || end == std::string::npos)
        goto failed;

    _part.name = tmp.substr(pos + 6, end - (pos + 6));
    _state = ST_DATA;

    pos = tmp.find("filename=\"");
    end = tmp.find("\"", pos + 10);

    if (pos == std::string::npos)
        return; // it could mean it is just not a file upload

    if (end == std::string::npos)
        goto failed;

    _part.filename = tmp.substr(pos + 10, end - (pos + 10));
    _part.filePath = _uploadDict + '/' + _part.filename;
    // the 'ios::binary' prevents the automatic char conversions
    // e.g: '\n' becomse a new line, but if ios::binary is added
    // it outputs it directly as it is
    _outfile.open(_part.filePath.c_str(), std::ios::binary | std::ios::out);
    if (!_outfile.is_open())
    {
        _logger.error("could open file for upload: " + _part.filePath);
        goto failed;
    }
    _part.isfile = true;

    return;
failed:
    _onError();
}
void    Multipart::_parseData()
{
    size_t s = _buff.peek(_tmp_buff, sizeof(_tmp_buff));
    if (!s) return;

    char* bound = std::search(
        _tmp_buff,
        _tmp_buff + sizeof(_tmp_buff),
        _str_boundry.data(),
        _str_boundry.data() + _str_boundry.size()
    );

    // means the bound was found
    if (bound != _tmp_buff + sizeof(_tmp_buff))
    {
        _buff.advanceRead(bound - _tmp_buff);
        _state = ST_SAVEPART;
    }
    else
        _buff.advanceRead(s);
    
    _handleBody(bound - _tmp_buff);
}
void    Multipart::_handleBody(size_t size)
{
    if (_state == ST_SAVEPART)
        size -= 2; // to remove the CRLF before the boundry
    if (_part.isfile)
    {
        _outfile.write(_tmp_buff, size);
        // std::cout << "chunk to file: ";
        // std::cout.write(_tmp_buff, size);
        return;
    }
    _part.body.write(_tmp_buff, size);
    // std::cout << "chunk to body: ";
    // std::cout.write(_tmp_buff, size);
    
}

void    Multipart::_onError()
{
    _logger.error("parsing error on multipart/form-data");    
    reset();
    _state = ST_MERROR;
}

void    Multipart::reset()
{
    _state = ST_SEEKBOUND;
    _parts.clear();
    _str_boundry.clear();
    _uploadDict.clear();
    _outfile.close();
    _resetPart();
}
void    Multipart::_resetPart()
{
    _part.body.clear();
    _part.filename.clear();
    _part.filePath.clear();
    _part.isfile = false;
    _part.name.clear();
}
void    Multipart::_savePart()
{
    _state = ST_SEEKBOUND;
    _parts.push_back(_part);
    _resetPart();
}

// int main()
// {
//     char test[] =
//     "--------------------------840a7da8fa23b607" CRLF
//     "Content-Disposition: form-data; name=\"file\"; filename=\"photo.jpg\"" CRLF
//     "Content-Type: image/jpeg" CRLF
//     CRLF
//     "[BINARY DATA]" CRLF
//     "--------------------------840a7da8fa23b607" CRLF
//     "Content-Disposition: form-data; name=\"bio\"" CRLF
//     CRLF
//     "this is just a dummy text" CRLF
//     "--------------------------840a7da8fa23b607" CRLF
//     "Content-Disposition: form-data; name=\"body\"; filename=\"photo.jpg\"" CRLF
//     "Content-Type: image/jpeg" CRLF
//     CRLF
//     "[BINARY DATA]" CRLF
//     "--------------------------840a7da8fa23b607--" CRLF;

//     RingBuffer bufftest(BUFSIZ);
//     bufftest.write(test, sizeof(test));

//     Multipart parser(bufftest);
//     parser.setBoundry("------------------------840a7da8fa23b607");

//     parser.setUploadPath("./");
//     parser.parse();

//     Multipart::parts_t &parts = parser.getParts();
//     std::cout << "number of parts: " << parts.size() << std::endl;
//     std::cout << "is complete    : " << parser.isComplete() << std::endl;
//     std::cout << "is Error       : " << parser.isError() << std::endl;

//     for (size_t i = 0; i < parts.size(); ++i)
//     {
//         std::cout << "------------------------------\n";
//         std::cout << parts[i].name << std::endl;
//         if (parts[i].isfile)
//         {
//         std::cout << parts[i].filename << std::endl;
//         std::cout << parts[i].filePath << std::endl;
//         }
//     }

// }