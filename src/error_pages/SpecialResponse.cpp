#include "SpecialResponse.hpp"

#define CRLF        "\r\n"
#define WEBSERV_VER "WebServ 1.0"

std::map<int, std::string> defaultErrorPages;

static std::string webserv_error_full_tail =
"<hr><center>" WEBSERV_VER "</center>" CRLF
"</body>" CRLF
"</html>" CRLF
;

static std::string emptyPage =
"<html>" CRLF
"<head><title>ERROR</title></head>" CRLF
"<body>" CRLF
"<center><h1>Unknown Error Code</h1></center>" CRLF
;

void initErrorPages()
{
    defaultErrorPages[301] =
    "<html>" CRLF
    "<head><title>301 Moved Permanently</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>301 Moved Permanently</h1></center>" CRLF
    ;

    defaultErrorPages[302] =
    "<html>" CRLF
    "<head><title>302 Found</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>302 Found</h1></center>" CRLF
    ;

    defaultErrorPages[303] =
    "<html>" CRLF
    "<head><title>303 See Other</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>303 See Other</h1></center>" CRLF
    ;

    defaultErrorPages[307] =
    "<html>" CRLF
    "<head><title>307 Temporary Redirect</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>307 Temporary Redirect</h1></center>" CRLF
    ;

    defaultErrorPages[308] =
    "<html>" CRLF
    "<head><title>308 Permanent Redirect</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>308 Permanent Redirect</h1></center>" CRLF
    ;

    defaultErrorPages[400] =
    "<html>" CRLF
    "<head><title>400 Bad Request</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>400 Bad Request</h1></center>" CRLF
    ;

    defaultErrorPages[401] =
    "<html>" CRLF
    "<head><title>401 Authorization Required</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>401 Authorization Required</h1></center>" CRLF
    ;

    defaultErrorPages[402] =
    "<html>" CRLF
    "<head><title>402 Payment Required</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>402 Payment Required</h1></center>" CRLF
    ;

    defaultErrorPages[403] =
    "<html>" CRLF
    "<head><title>403 Forbidden</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>403 Forbidden</h1></center>" CRLF
    ;

    defaultErrorPages[404] =
    "<html>" CRLF
    "<head><title>404 Not Found</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>404 Not Found</h1></center>" CRLF
    ;

    defaultErrorPages[405] =
    "<html>" CRLF
    "<head><title>405 Not Allowed</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>405 Not Allowed</h1></center>" CRLF
    ;

    defaultErrorPages[406] =
    "<html>" CRLF
    "<head><title>406 Not Acceptable</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>406 Not Acceptable</h1></center>" CRLF
    ;

    defaultErrorPages[408] =
    "<html>" CRLF
    "<head><title>408 Request Time-out</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>408 Request Time-out</h1></center>" CRLF
    ;

    defaultErrorPages[409] =
    "<html>" CRLF
    "<head><title>409 Conflict</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>409 Conflict</h1></center>" CRLF
    ;

    defaultErrorPages[410] =
    "<html>" CRLF
    "<head><title>410 Gone</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>410 Gone</h1></center>" CRLF
    ;

    defaultErrorPages[411] =
    "<html>" CRLF
    "<head><title>411 Length Required</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>411 Length Required</h1></center>" CRLF
    ;

    defaultErrorPages[412] =
    "<html>" CRLF
    "<head><title>412 Precondition Failed</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>412 Precondition Failed</h1></center>" CRLF
    ;

    defaultErrorPages[413] =
    "<html>" CRLF
    "<head><title>413 Request Entity Too Large</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>413 Request Entity Too Large</h1></center>" CRLF
    ;

    defaultErrorPages[414] =
    "<html>" CRLF
    "<head><title>414 Request-URI Too Large</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>414 Request-URI Too Large</h1></center>" CRLF
    ;

    defaultErrorPages[415] =
    "<html>" CRLF
    "<head><title>415 Unsupported Media Type</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>415 Unsupported Media Type</h1></center>" CRLF
    ;

    defaultErrorPages[416] =
    "<html>" CRLF
    "<head><title>416 Requested Range Not Satisfiable</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>416 Requested Range Not Satisfiable</h1></center>" CRLF
    ;

    defaultErrorPages[421] =
    "<html>" CRLF
    "<head><title>421 Misdirected Request</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>421 Misdirected Request</h1></center>" CRLF
    ;

    defaultErrorPages[429] =
    "<html>" CRLF
    "<head><title>429 Too Many Requests</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>429 Too Many Requests</h1></center>" CRLF
    ;

    defaultErrorPages[494] =
    "<html>" CRLF
    "<head><title>400 Request Header Or Cookie Too Large</title></head>"
    CRLF
    "<body>" CRLF
    "<center><h1>400 Bad Request</h1></center>" CRLF
    "<center>Request Header Or Cookie Too Large</center>" CRLF
    ;

    defaultErrorPages[495] =
    "<html>" CRLF
    "<head><title>400 The SSL certificate error</title></head>"
    CRLF
    "<body>" CRLF
    "<center><h1>400 Bad Request</h1></center>" CRLF
    "<center>The SSL certificate error</center>" CRLF
    ;

    defaultErrorPages[496] =
    "<html>" CRLF
    "<head><title>400 No required SSL certificate was sent</title></head>"
    CRLF
    "<body>" CRLF
    "<center><h1>400 Bad Request</h1></center>" CRLF
    "<center>No required SSL certificate was sent</center>" CRLF
    ;

    defaultErrorPages[497] =
    "<html>" CRLF
    "<head><title>400 The plain HTTP request was sent to HTTPS port</title></head>"
    CRLF
    "<body>" CRLF
    "<center><h1>400 Bad Request</h1></center>" CRLF
    "<center>The plain HTTP request was sent to HTTPS port</center>" CRLF
    ;

    defaultErrorPages[500] =
    "<html>" CRLF
    "<head><title>500 Internal Server Error</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>500 Internal Server Error</h1></center>" CRLF
    ;

    defaultErrorPages[501] =
    "<html>" CRLF
    "<head><title>501 Not Implemented</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>501 Not Implemented</h1></center>" CRLF
    ;

    defaultErrorPages[502] =
    "<html>" CRLF
    "<head><title>502 Bad Gateway</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>502 Bad Gateway</h1></center>" CRLF
    ;

    defaultErrorPages[503] =
    "<html>" CRLF
    "<head><title>503 Service Temporarily Unavailable</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>503 Service Temporarily Unavailable</h1></center>" CRLF
    ;

    defaultErrorPages[504] =
    "<html>" CRLF
    "<head><title>504 Gateway Time-out</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>504 Gateway Time-out</h1></center>" CRLF
    ;

    defaultErrorPages[505] =
    "<html>" CRLF
    "<head><title>505 HTTP Version Not Supported</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>505 HTTP Version Not Supported</h1></center>" CRLF
    ;

    defaultErrorPages[507] =
    "<html>" CRLF
    "<head><title>507 Insufficient Storage</title></head>" CRLF
    "<body>" CRLF
    "<center><h1>507 Insufficient Storage</h1></center>" CRLF
    ;
}



const std::string getErrorPage(int code)
{
    const std::map<int, std::string>::iterator& it = defaultErrorPages.find(code);
    const std::string& page = (it != defaultErrorPages.end()) ? it->second : emptyPage;
    return page + webserv_error_full_tail;
}
