#include "Parsing.hpp"

Server::Server() {
    ip = "127.0.0.1";
    port = 80;
    name = "localip";
    root = "/";
    files.push_back("index.html");
    maxBody = 10485760;
    errors[400] = "<html><body><h1>400 Bad Request</h1></body></html>";
    errors[403] = "<html><body><h1>403 Forbidden</h1></body></html>";
    errors[404] = "<html><body><h1>404 Not Found</h1></body></html>";
    errors[500] = "<html><body><h1>500 Internal Server Error</h1></body></html>";
}

void Location::ApplyDefaults(Server server) {
    route = "";
    root = server.root;
    cgi = "";
    redirect = "";
    upload = "";
    autoindex = false;
    methods.push_back("GET");
    maxBody = server.maxBody;
    files = server.files;
}
