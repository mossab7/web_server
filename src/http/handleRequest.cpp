#include "ConfigParser.hpp"
#include "Routing.hpp"
#include "HTTPParser.hpp"
#include "Response.hpp"
#include "error_pages.hpp"

HTTPResponse createErrorResponse(int code)
{
    cout << "Erroreeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee: [" << code << "]" << endl;

    HTTPResponse resp(code);
    resp.addHeader("Content-Type", "text/html");
    resp.setBody(getErrorPage(code));
    resp.endHeaders();
    return (resp);
}

HTTPResponse handleRequest(Routing &routing, const string &host, const string &request_path, const string &method)
{
    cout << "111111111111111" << endl;
    Server *server = routing.findServer(host);
    if (!server)
        return (createErrorResponse(404));
    cout << "222222222222222" << endl;
    Location *loc = routing.findLocation(*server, request_path);
    if (!loc)
        return (createErrorResponse(404));
    cout << "33333333333333333" << endl;

    if (!routing.isMethodAllowed(*loc, method))
        return (createErrorResponse(405));
    cout << "44444444444444444444" << endl;

    if (!loc->redirect.empty())
    {
        HTTPResponse resp(301, "Moved Permanently");
        resp.addHeader("Location", loc->redirect);
        resp.endHeaders();
        return (resp);
    }
    cout << "555555555555555555555" << endl;

    string filepath = loc->root + request_path;
    HTTPResponse resp;
    if (!resp.attachFile(filepath))

    cout << "Good Requesteeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee" << endl;

    resp.addHeader("Content-Type", "text/html");
    resp.endHeaders();
    return (resp);
}
