#include "ConfigParser.hpp"

ServerConfig::ServerConfig()
{
    initErrorPages();
    host = "127.0.0.1";
    port = 8080;
    name = "localhost:8080";
    root = "/";
    indexFiles.push_back("index.html");
    maxBody = 10485760;
    client_timeout = 60;
    errors[400] = getErrorPage(400);
    errors[403] = getErrorPage(403);
    errors[404] = getErrorPage(404);
    errors[500] = getErrorPage(500);
}



Location::Location(ServerConfig server)
{
    route = "";
    root = server.root;
    cgi = "";
    scriptInterpreter = "";
    cgi_timeout = 1000;
    redirect = "";
    upload = "";
    autoindex = false;
    methods.push_back("GET");
    maxBody = server.maxBody;
    client_timeout = server.client_timeout;
    indexFiles = server.indexFiles;
}



vector<ServerConfig> &WebConfigFile::getServers()
{
    return (_servers);
}



ServerConfig WebConfigFile::getServer(const string &name)
{
    for (size_t i = 0; i < _servers.size(); i++)
    {
        if (_servers[i].name == name)
            return (_servers[i]);
    }

    return (ServerConfig());
}



void WebConfigFile::addServer(const ServerConfig &server)
{
    _servers.push_back(server);
}



string trim(const string &str)
{
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == string::npos)
        return ("");
    size_t end = str.find_last_not_of(" \t\n\r");
    return (str.substr(start, end - start + 1));
}



string reduceSpaces(const string &str)
{
    string result;
    bool inSpace = false;
    char c = '\0';

    for (size_t i = 0; i < str.size(); ++i)
    {
        if ((str[i] == '\'' || str[i] == '\"') && (c != '\'' && c != '"'))
            c = str[i];
        else if ((str[i] == '\'' || str[i] == '\"') && (c == str[i]))
            c = '\0';

        if ((str[i] == ' ' || str[i] == '\t') && (c != '\'' && c != '\"'))
        {
            if (!inSpace)
            {
                result += ' ';
                inSpace = true;
            }
        }
        else
        {
            result += str[i];
            inSpace = false;
        }
    }
    return (result);
}



string removeComment(const string &str)
{
    size_t pos = str.find('#');
    if (pos != string::npos)
        return (reduceSpaces(trim(str.substr(0, pos))));
    return (reduceSpaces(trim(str)));
}



vector<string> split(const string &str)
{
    vector<string> tokens;
    string current;
    char c = '\0';

    for (size_t i = 0; i < str.size(); i++)
    {
        if ((str[i] == '\'' || str[i] == '"') && c == '\0')
            c = str[i];

        else if ((str[i] == '\'' || str[i] == '"') && c == str[i])
            c = '\0';

        else if (str[i] == ' ' && c == '\0')
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
        }
        else
            current += str[i];
    }

    if (!current.empty())
        tokens.push_back(current);

    return (tokens);
}



void throwSyntaxError(string &str, const string &fname, size_t &lnNbr)
{
    ostringstream oss;
    oss << "Webserv: syntax error in " << fname << " at line " << lnNbr << " → " << str;
    throw(runtime_error(oss.str()));
}



size_t myAtol(string str, string &line, const string &fname, size_t &lnNbr)
{
    for (size_t i = 0; i < str.size(); i++)
    {
        if (!isdigit(str[i]))
            throwSyntaxError(line, fname, lnNbr);
    }
    return (atol(str.c_str()));
}



short handleLocation(string str, vector<string> &tokens, Location &locTmp, const string &fname, size_t &lnNbr)
{
    if (tokens.size() < 2)
        throwSyntaxError(str, fname, lnNbr);

    if (tokens.size() == 2 && tokens[0] == "route")
        locTmp.route = tokens[1];

    else if (tokens.size() == 2 && tokens[0] == "root")
        locTmp.root = tokens[1];

    else if (tokens.size() == 2 && tokens[0] == "autoindex")
    {
        if (tokens[1] == "on")
            locTmp.autoindex = true;
        else if (tokens[1] == "off")
            locTmp.autoindex = false;
        else
            throwSyntaxError(str, fname, lnNbr);
    }

    else if (tokens.size() == 2 && tokens[0] == "client_max_body_size")
        locTmp.maxBody = myAtol(tokens[1], str, fname, lnNbr);

    else if (tokens.size() == 2 && tokens[0] == "client_timeout")
        locTmp.client_timeout = myAtol(tokens[1], str, fname, lnNbr);

    else if (tokens.size() == 2 && tokens[0] == "redirect")
        locTmp.redirect = tokens[1];

    else if (tokens.size() == 2 && tokens[0] == "upload_store")
        locTmp.upload = tokens[1];

    else if (tokens.size() == 2 && tokens[0] == "cgi_pass")
        locTmp.cgi = tokens[1];

    else if (tokens.size() == 2 && tokens[0] == "script_interpreter")
        locTmp.scriptInterpreter = tokens[1];

    else if (tokens.size() == 2 && tokens[0] == "cgi_timeout")
        locTmp.cgi_timeout = myAtol(tokens[1], str, fname, lnNbr);

    else if (tokens[0] == "index")
    {
        locTmp.indexFiles.clear();
        for (size_t i = 1; i < tokens.size(); i++)
        {
            for (size_t j = 0; j < locTmp.indexFiles.size(); j++)
            {
                if (tokens[i] == locTmp.indexFiles[j])
                    throwSyntaxError(str, fname, lnNbr);
            }
            locTmp.indexFiles.push_back(tokens[i]);
        }
    }

    else if (tokens[0] == "methods")
    {
        locTmp.methods.clear();
        for (size_t i = 1; i < tokens.size(); i++)
        {
            for (size_t j = 0; j < locTmp.methods.size(); j++)
            {
                if (tokens[i] == locTmp.methods[j])
                    throwSyntaxError(str, fname, lnNbr);
            }

            if (tokens[i] != "GET" && tokens[i] != "POST" && tokens[i] != "DELETE")
                throwSyntaxError(str, fname, lnNbr);
            locTmp.methods.push_back(tokens[i]);
        }
    }

    else
        throwSyntaxError(str, fname, lnNbr);

    return (0);
}



short handleServer(string str, vector<string> &tokens, ServerConfig &srvTmp, const string &fname, size_t &lnNbr)
{
    if (tokens.size() < 2)
        throwSyntaxError(str, fname, lnNbr);

    if (tokens.size() == 2 && tokens[0] == "host")
    {
        struct in_addr addr;
        if (!inet_aton(tokens[1].c_str(), &addr))
            throwSyntaxError(str, fname, lnNbr);
        srvTmp.host = tokens[1];
    }

    else if (tokens.size() == 2 && tokens[0] == "port")
    {
        srvTmp.port = myAtol(tokens[1], str, fname, lnNbr);
        if (srvTmp.port > 65535)
            throwSyntaxError(str, fname, lnNbr);
    }

    else if (tokens.size() == 2 && tokens[0] == "server_name")
        srvTmp.name = tokens[1];

    else if (tokens.size() == 2 && tokens[0] == "root")
        srvTmp.root = tokens[1];

    else if (tokens.size() == 2 && tokens[0] == "client_max_body_size")
        srvTmp.maxBody = myAtol(tokens[1], str, fname, lnNbr);

    else if (tokens.size() == 2 && tokens[0] == "client_timeout")
        srvTmp.client_timeout = myAtol(tokens[1], str, fname, lnNbr);

    else if (tokens[0] == "index")
    {
        srvTmp.indexFiles.clear();
        for (size_t i = 1; i < tokens.size(); i++)
        {
            for (size_t j = 0; j < srvTmp.indexFiles.size(); j++)
            {
                if (tokens[i] == srvTmp.indexFiles[j])
                    throwSyntaxError(str, fname, lnNbr);
            }
            srvTmp.indexFiles.push_back(tokens[i]);
        }
    }

    else if (tokens.size() == 3 && tokens[0] == "error_page")
        srvTmp.errors[myAtol(tokens[1], str, fname, lnNbr)] = tokens[2];

    else
        throwSyntaxError(str, fname, lnNbr);

    return (0);
}



short handleDirective(string &str, const string &fName, size_t &lnNbr, WebConfigFile &config)
{
    static bool srvActive = false;
    static bool inLocation = false;
    static ServerConfig srvTmp;
    static Location locTmp(srvTmp);

    vector<string> tokens = split(str);
    if (tokens.empty())
        return (0);

    if ((tokens.size() == 1 && tokens[0] == "server{") ||
        (tokens.size() == 2 && tokens[0] == "server" && tokens[1] == "{"))
    {
        if (srvActive)
            throwSyntaxError(str, fName, lnNbr);
        srvActive = true;
        srvTmp = ServerConfig();
        return (0);
    }

    if ((tokens.size() == 1 && tokens[0] == "location{") ||
        (tokens.size() == 2 && tokens[0] == "location" && tokens[1] == "{"))
    {
        if (!srvActive || inLocation)
            throwSyntaxError(str, fName, lnNbr);
        inLocation = true;
        locTmp = Location(srvTmp);
        return (0);
    }

    if (tokens[0] == "}")
    {
        if (inLocation)
        {
            if (locTmp.route == "")
                throwSyntaxError(str, fName, lnNbr);
            srvTmp.locations.push_back(locTmp);
            inLocation = false;
        }
        else if (srvActive)
        {
            ostringstream port;
            port << srvTmp.port;
            if (srvTmp.name == "")
                srvTmp.name = srvTmp.host + ":" + port.str();
            else
                srvTmp.name = srvTmp.name + ":" + port.str();
            config.addServer(srvTmp);
            srvActive = false;
        }
        else
            throwSyntaxError(str, fName, lnNbr);
        return (0);
    }

    if (inLocation)
        return (handleLocation(str, tokens, locTmp, fName, lnNbr));
    else if (srvActive)
        return (handleServer(str, tokens, srvTmp, fName, lnNbr));
    else
        throwSyntaxError(str, fName, lnNbr);

    return (0);
}



WebConfigFile::WebConfigFile(const string &fName)
{
    _inputFile.open(fName.c_str());
    if (!_inputFile.is_open())
        throw runtime_error("Error: Cannot open config file " + fName);

    string currentLine;
    size_t lnNbr = 0;

    while (getline(_inputFile, currentLine))
    {
        ++lnNbr;
        currentLine = removeComment(currentLine);
        if (currentLine.empty())
            continue;

        handleDirective(currentLine, fName, lnNbr, *this);
    }

    if (lnNbr == 0)
        throw runtime_error("Error: Configuration file is empty " + fName);
}



WebConfigFile::~WebConfigFile()
{
    if (_inputFile.is_open())
        _inputFile.close();
}





