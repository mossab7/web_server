#include "Parsing.hpp"

short parseConfigFile(WebConfigFile &config, const string &fname) {
    ifstream inputFile(fname.c_str());
    if (!inputFile.is_open()) {
        cerr << "Error: Cannot open config file " << fname << endl;
        return (1);
    }
    string currentLine;
    size_t lnNbr = 0;
    while (getline(inputFile, currentLine)) {
        ++lnNbr;
        currentLine = removeComment(currentLine);
        if (currentLine.empty())
            continue;

        if (handleDirective(currentLine, fname, lnNbr, config))
            return (1);
    }
    if (lnNbr == 0) {
        cerr << "Error: Configuration file is empty" << endl;
        return (1);
    }
    inputFile.close();
    return (0);
}

string removeComment(const string &str) {
    size_t  pos = str.find('#');
    if (pos != string::npos)
        return (reduceSpaces(trim(str.substr(0, pos))));
    return (reduceSpaces(trim(str)));
}

short handleDirective(string &str, const string &fname, size_t &lnNbr, WebConfigFile &config) {
    static bool srvActive = false;
    static bool inLocation = false;
    static Server srvTmp;
    static Location locTmp;

    vector<string> tokens = split(str);
    if (tokens.empty())
        return (0);

    if ((tokens.size() == 1 && tokens[0] == "server{") || 
        (tokens.size() == 2 && tokens[0] == "server" && tokens[1] == "{")) {
        if (srvActive)
            return printError(str, fname, lnNbr);
        srvActive = true;
        srvTmp = Server();
        return (0);
    }

    if ((tokens.size() == 1 && tokens[0] == "location{") || 
        (tokens.size() == 2 && tokens[0] == "location" && tokens[1] == "{")) {
        if (!srvActive || inLocation)
            return printError(str, fname, lnNbr);
        inLocation = true;
        locTmp = Location();
        locTmp.ApplyDefaults(srvTmp);
        return (0);
    }

    if (tokens[0] == "}") {
        if (inLocation) {
            if (locTmp.route == "")
                return printError(str, fname, lnNbr);
            srvTmp.locations.push_back(locTmp);
            inLocation = false;
        }
        else if (srvActive) {
            config.servers.push_back(srvTmp);
            srvActive = false;
        }
        else
            return printError(str, fname, lnNbr);
        return (0);
    }

    if (inLocation)
        return handleLocation(str, tokens, locTmp, fname, lnNbr);
    else if (srvActive)
        return handleServer(str, tokens, srvTmp, fname, lnNbr);
    else
        return printError(str, fname, lnNbr);

    return (0);
}

short printError(string &str, const string &fname, size_t &lnNbr) {
    cerr << "Webserv: syntax error in " << fname << " at line " << lnNbr << " → " << str << endl;
    return (1);
}

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
