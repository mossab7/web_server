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

string trim(const string &str) {
    size_t  start = str.find_first_not_of(" \t\n\r");
    if (start == string::npos)
        return ("");
    size_t  end = str.find_last_not_of(" \t\n\r");
    return (str.substr(start, end - start + 1));
}

string reduceSpaces(const string &str) {
    string result;
    bool    inSpace = false;
    char    c = '\0';

    for (size_t i = 0; i < str.size(); ++i) {
        if ((str[i] == '\'' || str[i] == '\"') && (c != '\'' && c != '"'))
            c = str[i];
        else if ((str[i] == '\'' || str[i] == '\"') && (c == str[i]))
            c = '\0';

        if ((str[i] == ' ' || str[i] == '\t') && (c != '\'' && c != '\"')) {
            if (!inSpace) {
                result += ' ';
                inSpace = true;
            }
        }
        else {
            result += str[i];
            inSpace = false;
        }
    }
    return (result);
}

vector<string> split(const string &str) {
    vector<string>    tokens;
    string current;
    char    c = '\0';

    for (size_t i = 0; i < str.size();i++) {
        if ((str[i] == '\'' || str[i] == '"') && c == '\0')
            c = str[i];

        else if ((str[i] == '\'' || str[i] == '"') && c == str[i])
            c = '\0';

        else if (str[i] == ' ' && c == '\0') {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        }
        else
            current += str[i];
    }

    if (!current.empty())
        tokens.push_back(current);

    return tokens;
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

short handleLocation(string str, vector<string> &tokens, Location &locTmp, const string &fname, size_t &lnNbr) {
    if (tokens.size() < 2)
        return (printError(str, fname, lnNbr));

    if (tokens.size() == 2 && tokens[0] == "route")
        locTmp.route = tokens[1];

    else if (tokens.size() == 2 && tokens[0] == "root")
        locTmp.root = tokens[1];

    else if (tokens.size() == 2 && tokens[0] == "autoindex") {
        if (tokens[1] == "on")
            locTmp.autoindex = true;
        else if (tokens[1] == "off")
            locTmp.autoindex = false;
        else
            return (printError(str, fname, lnNbr));
    }

    else if (tokens.size() == 2 && tokens[0] == "client_max_body_size")
        locTmp.maxBody = myAtol(tokens[1], str, fname, lnNbr);

    else if (tokens.size() == 2 && tokens[0] == "redirect")
        locTmp.redirect = tokens[1];

    else if (tokens.size() == 2 && tokens[0] == "upload_store")
        locTmp.upload = tokens[1];

    else if (tokens.size() == 2 && tokens[0] == "cgi_pass")
        locTmp.cgi = tokens[1];

    else if (tokens[0] == "index") {
        locTmp.files.clear();
        for (size_t i = 1; i < tokens.size(); i++) {
            for (size_t j = 0; j < locTmp.files.size(); j++) {
                if (tokens[i] == locTmp.files[j])
                    return printError(str, fname, lnNbr);
            }
            locTmp.files.push_back(tokens[i]);
        }
    }

    else if (tokens[0] == "methods") {
        locTmp.methods.clear();
        for (size_t i = 1; i < tokens.size(); i++) {
            for (size_t j = 0; j < locTmp.methods.size(); j++) {
                if (tokens[i] == locTmp.methods[j])
                    return printError(str, fname, lnNbr);
            }

            if (tokens[i] != "GET" && tokens[i] != "POST" && tokens[i] != "DELETE")
                return printError(str, fname, lnNbr);
            locTmp.methods.push_back(tokens[i]);
        }
    }

    else
        return (printError(str, fname, lnNbr));

    return (0);
}

short handleServer(string str, vector<string> &tokens, Server &srvTmp, const string &fname, size_t &lnNbr) {
    if (tokens.size() < 2)
        return (printError(str, fname, lnNbr));

    if (tokens.size() == 2 && tokens[0] == "host") {
        struct in_addr  addr;
        if (!inet_aton(tokens[1].c_str(), &addr))
            return (printError(str, fname, lnNbr));
        srvTmp.ip = tokens[1];
    }

    else if (tokens.size() == 2 && tokens[0] == "port")
    {
        srvTmp.port = myAtol(tokens[1], str, fname, lnNbr);
        if (srvTmp.port > 65535)
            return (printError(str, fname, lnNbr));
    }

    else if (tokens.size() == 2 && tokens[0] == "server_name")
        srvTmp.name = tokens[1];

    else if (tokens.size() == 2 && tokens[0] == "root")
        srvTmp.root = tokens[1];

    else if (tokens.size() == 2 && tokens[0] == "client_max_body_size")
        srvTmp.maxBody = myAtol(tokens[1], str, fname, lnNbr);

    else if (tokens[0] == "index") {
        srvTmp.files.clear();
        for (size_t i = 1; i < tokens.size(); i++) {
            for (size_t j = 0; j < srvTmp.files.size(); j++) {
                if (tokens[i] == srvTmp.files[j])
                    return printError(str, fname, lnNbr);
            }
            srvTmp.files.push_back(tokens[i]);
        }
    }

    else if (tokens.size() == 3 && tokens[0] == "error_page")
        srvTmp.errors[myAtol(tokens[1], str, fname, lnNbr)] = tokens[2];

    else
        return (printError(str, fname, lnNbr));

    return (0);
}

short printError(string &str, const string &fname, size_t &lnNbr) {
    cerr << "Webserv: syntax error in " << fname << " at line " << lnNbr << " → " << str << endl;
    return (1);
}

size_t myAtol(string str, string &line, const string &fname, size_t &lnNbr) {
    for (size_t i = 0; i < str.size(); i++) {
        if (!isdigit(str[i]))
            return (printError(line, fname, lnNbr));
    }
    return (atol(str.c_str()));
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
