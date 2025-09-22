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
    Server *server = routing.findServer(host);
    if (!server)
        return (createErrorResponse(404));

    Location *loc = routing.findLocation(*server, request_path);
    if (!loc)
        return (createErrorResponse(404));

    if (!routing.isMethodAllowed(*loc, method))
        return (createErrorResponse(405));

    if (!loc->redirect.empty())
    {
        HTTPResponse resp(301, "Moved Permanently");
        resp.addHeader("Location", loc->redirect);
        resp.endHeaders();
        return (resp);
    }

    cout << "Good Requesteeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee" << endl;

    HTTPResponse resp;
    resp.addHeader("Content-Type", "text/html");
    resp.endHeaders();
    return (resp);
}
