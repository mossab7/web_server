#include "error_pages.hpp"

/**
 * @brief Global map holding default HTML pages for common HTTP error codes.
 *
 * This allows centralized management of error pages for the web server.
 * Each key is an HTTP status code (int), and the value is the HTML string
 * representing the error page.
 */
map<int, string> defaultErrorPages;

/**
 * @brief Initialize the defaultErrorPages map with standard HTTP error pages.
 *
 * This function fills the defaultErrorPages map with simple HTML pages for
 * common HTTP error codes (400, 403, 404, 405, 408, 413, 414, 500, 501, etc...).
 * Should be called once during server startup before handling requests.
 */
void initErrorPages()
{
      defaultErrorPages[400] = "<html><head><title>400 Bad Request</title></head>"
                               "<body><center><h1>400 Bad Request</h1></center>"
                               "<hr><center>webserv</center></body></html>";

      defaultErrorPages[403] = "<html><head><title>403 Forbidden</title></head>"
                               "<body><center><h1>403 Forbidden</h1></center>"
                               "<hr><center>webserv</center></body></html>";

      defaultErrorPages[404] = "<html><head><title>404 Not Found</title></head>"
                               "<body><center><h1>404 Not Found</h1></center>"
                               "<hr><center>webserv</center></body></html>";

      defaultErrorPages[405] = "<html><head><title>405 Method Not Allowed</title></head>"
                               "<body><center><h1>405 Method Not Allowed</h1></center>"
                               "<hr><center>webserv</center></body></html>";

      defaultErrorPages[408] = "<html><head><title>408 Request Timeout</title></head>"
                               "<body><center><h1>408 Request Timeout</h1></center>"
                               "<hr><center>webserv</center></body></html>";

      defaultErrorPages[413] = "<html><head><title>413 Payload Too Large</title></head>"
                               "<body><center><h1>413 Payload Too Large</h1></center>"
                               "<hr><center>webserv</center></body></html>";

      defaultErrorPages[414] = "<html><head><title>414 URI Too Long</title></head>"
                               "<body><center><h1>414 URI Too Long</h1></center>"
                               "<hr><center>webserv</center></body></html>";

      defaultErrorPages[500] = "<html><head><title>500 Internal Server Error</title></head>"
                               "<body><center><h1>500 Internal Server Error</h1></center>"
                               "<hr><center>webserv</center></body></html>";

      defaultErrorPages[501] = "<html><head><title>501 Not Implemented</title></head>"
                               "<body><center><h1>501 Not Implemented</h1></center>"
                               "<hr><center>webserv</center></body></html>";

      defaultErrorPages[502] = "<html><head><title>502 Bad Gateway</title></head>"
                               "<body><center><h1>502 Bad Gateway</h1></center>"
                               "<hr><center>webserv</center></body></html>";

      defaultErrorPages[503] = "<html><head><title>503 Service Unavailable</title></head>"
                               "<body><center><h1>503 Service Unavailable</h1></center>"
                               "<hr><center>webserv</center></body></html>";
}

/**
 * @brief Get the HTML page for a given HTTP error code.
 *
 * If the code exists in defaultErrorPages, return the corresponding HTML.
 * Otherwise, return a default "Unknown Error" HTML page.
 *
 * @param code HTTP status code
 * @return const string& Reference to the HTML content
 */
const string &getErrorPage(int code)
{
      static string emptyPage =
          "<html><head><title>Error</title></head>"
          "<body><center><h1>Unknown Error</h1></center>"
          "<hr><center>webserv</center></body></html>";

      if (defaultErrorPages.find(code) != defaultErrorPages.end())
            return defaultErrorPages[code];

      return emptyPage;
}
