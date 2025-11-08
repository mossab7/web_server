#ifndef WEBSERV_MULTIPART_HPP
#define WEBSERV_MULTIPART_HPP

#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#include "RingBuffer.hpp"
#include "Logger.hpp"

#define CRLF "\r\n"

enum multipartState
{
    ST_SEEKBOUND  ,
    ST_HEADERS    ,
    ST_DATA       ,
    ST_SAVEPART   ,
    ST_COMPLETE   ,
    ST_MERROR
};

class Multipart
{
public:
    struct formData
    {
        std::string name;
        std::string filename;

        // just normal forms
        RingBuffer body;

        // file stuff
        std::string     filePath;
        bool            isfile;

        formData(): body(BUFSIZ), isfile(false) {}
    };
    typedef std::vector<formData> parts_t;

private:
    // this could lead to a bug if the boundry is bigger for example.
    // todo fix that. ( i think i will never see this code again XD )
    char    _tmp_buff[BUFSIZ];

    multipartState  _state;

    std::string     _str_boundry;
    std::string     _uploadDict;

    RingBuffer&     _buff;
    std::ofstream   _outfile;

    parts_t     _parts; // a vector of paths
    formData    _part; // a single part

    Logger      _logger;

    void    _onError();

    void    _seekBound();
    void    _parseHeaders();
    void    _parseData();

    void    _handleBody(size_t chunk_size);

    void    _resetPart();
    void    _savePart();

public:
    // i will inject the buffer
    Multipart(RingBuffer& body);
    ~Multipart();
    
    void    setUploadPath(const std::string& path);
    void    setBoundry(const std::string& bound);

    parts_t&   getParts();

    multipartState getState();

    bool    isComplete();
    bool    isError();

    void    parse();
    void    reset();
};

#endif