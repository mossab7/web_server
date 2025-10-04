#include "ConfigParser.hpp"

/**
 * @brief Default constructor for the Server struct.
 *
 * Initializes a server instance with default values:
 * - Host: "127.0.0.1" (localhost)
 * - Port: 8080
 * - Name: "localhost:8080"
 * - Root directory: "/"
 * - Default index file: "index.html"
 * - Maximum request body size: 10 MB
 * - Standard HTTP error pages (400, 403, 404, 500) are loaded using getErrorPage().
 *
 * Also calls initErrorPages() to ensure the error pages system is ready.
 */
Server::Server()
{
    initErrorPages();
    host = "127.0.0.1";
    port = 8080;
    name = "localhost:8080";
    root = "/";
    files.push_back("index.html");
    maxBody = 10485760;
    errors[400] = getErrorPage(400);
    errors[403] = getErrorPage(403);
    errors[404] = getErrorPage(404);
    errors[500] = getErrorPage(500);
}

/**
 * @brief Constructor for the Location struct, initializing from a Server instance.
 *
 * Sets default values for a location block:
 * - route: empty string
 * - root: inherited from the given Server
 * - cgi, redirect, upload: empty strings
 * - autoindex: false
 * - methods: default to "GET"
 * - maxBody: inherited from the Server's maxBody
 * - files: inherit server's default index files
 *
 * @param server The Server instance from which to inherit default values.
 */
Location::Location(Server server)
{
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

/**
 * @brief Returns a reference to the vector of servers in the configuration.
 *
 * This allows direct access to the list of Server objects stored in the
 * WebConfigFile, enabling iteration, modification, or queries.
 *
 * @return Reference to the internal vector of Server objects.
 */
vector<Server> &WebConfigFile::getServers()
{
    return (_servers);
}

/**
 * @brief Retrieves a server from the configuration by its name.
 *
 * This function searches through the internal list of servers (_servers)
 * and returns a copy of the Server object whose 'name' matches the provided
 * string. If no matching server is found, a default Server object is returned.
 *
 * @param name The name of the server to search for.
 * @return A copy of the matching Server object, or a default Server if none found.
 */
Server WebConfigFile::getServer(const string &name)
{
    for (size_t i = 0; i < _servers.size(); i++)
    {
        if (_servers[i].name == name)
            return (_servers[i]);
    }

    return (Server());
}

/**
 * @brief Adds a new server to the configuration.
 *
 * This function appends the given Server object to the internal list
 * of servers (_servers) maintained by the WebConfigFile.
 *
 * @param server The Server object to add to the configuration.
 */
void WebConfigFile::addServer(const Server &server)
{
    _servers.push_back(server);
}

/**
 * @brief Removes leading and trailing whitespace characters from a string.
 *
 * This function trims spaces, tabs, newlines, and carriage returns
 * from both the beginning and the end of the input string.
 *
 * @param str The input string to trim.
 * @return A new string with no leading or trailing whitespace.
 */
string trim(const string &str)
{
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == string::npos)
        return ("");
    size_t end = str.find_last_not_of(" \t\n\r");
    return (str.substr(start, end - start + 1));
}

/**
 * @brief Reduces multiple consecutive spaces or tabs to a single space.
 *
 * This function iterates through the input string and collapses sequences
 * of spaces and tabs into a single space, while preserving spaces inside
 * quotes (single or double).
 *
 * @param str The input string to process.
 * @return A new string with reduced whitespace between words.
 */
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

/**
 * @brief Removes comments from a configuration line.
 *
 * This function looks for the '#' character and treats everything after it
 * as a comment. It trims leading/trailing whitespace and reduces multiple
 * spaces/tabs between words. If no comment is found, it just trims and
 * reduces spaces in the entire line.
 *
 * @param str The input string from the configuration file.
 * @return A cleaned string with comments removed and extra spaces reduced.
 */
string removeComment(const string &str)
{
    size_t pos = str.find('#');
    if (pos != string::npos)
        return (reduceSpaces(trim(str.substr(0, pos))));
    return (reduceSpaces(trim(str)));
}

/**
 * @brief Splits a string into tokens based on spaces, respecting quotes.
 *
 * This function parses the input string and splits it into separate words
 * (tokens) using spaces as delimiters. Text enclosed in single ('') or
 * double ("") quotes is treated as a single token, even if it contains spaces.
 *
 * @param str The input string to split.
 * @return A vector of string tokens extracted from the input.
 */
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

    return tokens;
}

/**
 * @brief Prints a syntax error message for the configuration file.
 *
 * This function outputs a formatted error message to std::cerr indicating
 * the file name, line number, and the offending line where the syntax error
 * occurred.
 *
 * @param str The line content that caused the error.
 * @param fname The name of the configuration file being parsed.
 * @param lnNbr The line number where the error was detected.
 * @return Always returns 1 to indicate an error occurred.
 */
short printError(string &str, const string &fname, size_t &lnNbr)
{
    cerr << "Webserv: syntax error in " << fname << " at line " << lnNbr << " → " << str << endl;
    return (1);
}

/**
 * @brief Converts a numeric string to a size_t value.
 *
 * This function checks that the given string contains only digits and then
 * converts it to a size_t using atol. If any non-digit character is found,
 * it prints a syntax error using printError.
 *
 * @param str The string representing a numeric value.
 * @param line The full line from the configuration file (used for error reporting).
 * @param fname The name of the configuration file.
 * @param lnNbr The line number in the configuration file.
 * @return The converted numeric value, or calls printError if the string is invalid.
 */
size_t myAtol(string str, string &line, const string &fname, size_t &lnNbr)
{
    for (size_t i = 0; i < str.size(); i++)
    {
        if (!isdigit(str[i]))
            return (printError(line, fname, lnNbr));
    }
    return (atol(str.c_str()));
}

/**
 * @brief Parses and applies a single directive within a location block.
 *
 * This function examines the tokens of a configuration line and updates
 * the corresponding fields of the given Location object (locTmp). It handles
 * directives such as route, root, autoindex, client_max_body_size, redirect,
 * upload_store, cgi_pass, index, and methods.
 *
 * Validation:
 * - Ensures numeric values for client_max_body_size.
 * - Ensures methods are one of GET, POST, DELETE.
 * - Prevents duplicate entries in files and methods vectors.
 *
 * @param str The full line from the configuration file (used for error reporting).
 * @param tokens Tokenized words of the line.
 * @param locTmp The Location object to be updated.
 * @param fname The name of the configuration file.
 * @param lnNbr The line number in the configuration file.
 * @return 0 if successful, 1 if a syntax error occurs.
 */
short handleLocation(string str, vector<string> &tokens, Location &locTmp, const string &fname, size_t &lnNbr)
{
    if (tokens.size() < 2)
        return (printError(str, fname, lnNbr));

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

    else if (tokens[0] == "index")
    {
        locTmp.files.clear();
        for (size_t i = 1; i < tokens.size(); i++)
        {
            for (size_t j = 0; j < locTmp.files.size(); j++)
            {
                if (tokens[i] == locTmp.files[j])
                    return printError(str, fname, lnNbr);
            }
            locTmp.files.push_back(tokens[i]);
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

/**
 * @brief Parses and applies a single directive within a server block.
 *
 * This function processes the tokens of a configuration line and updates
 * the corresponding fields of the given Server object (srvTmp). It handles
 * directives such as host, port, server_name, root, client_max_body_size,
 * index, and error_page.
 *
 * Validation:
 * - Ensures a valid IPv4 address for host.
 * - Ensures port numbers are within the range 0–65535.
 * - Ensures client_max_body_size is numeric.
 * - Prevents duplicate entries in the index files vector.
 *
 * @param str The full line from the configuration file (used for error reporting).
 * @param tokens Tokenized words of the line.
 * @param srvTmp The Server object to be updated.
 * @param fname The name of the configuration file.
 * @param lnNbr The line number in the configuration file.
 * @return 0 if successful, 1 if a syntax error occurs.
 */
short handleServer(string str, vector<string> &tokens, Server &srvTmp, const string &fname, size_t &lnNbr)
{
    if (tokens.size() < 2)
        return (printError(str, fname, lnNbr));

    if (tokens.size() == 2 && tokens[0] == "host")
    {
        struct in_addr addr;
        if (!inet_aton(tokens[1].c_str(), &addr))
            return (printError(str, fname, lnNbr));
        srvTmp.host = tokens[1];
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

    else if (tokens[0] == "index")
    {
        srvTmp.files.clear();
        for (size_t i = 1; i < tokens.size(); i++)
        {
            for (size_t j = 0; j < srvTmp.files.size(); j++)
            {
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

/**
 * @brief Processes a configuration line and determines its context.
 *
 * This function identifies whether the current line belongs to a server
 * block, a location block, or is a closing brace. It maintains static
 * state to track whether the parser is currently inside a server or
 * location block.
 *
 * Behavior:
 * - Starts a new Server or Location object when encountering "server{" or "location{".
 * - Closes the current Location or Server when encountering "}".
 * - Delegates processing of directives to handleServer or handleLocation.
 * - Returns a syntax error if a directive appears outside of a block or if
 *   block syntax rules are violated.
 *
 * @param str The configuration line to process.
 * @param fName The name of the configuration file (for error reporting).
 * @param lnNbr The current line number (for error reporting).
 * @param config Reference to the WebConfigFile object being populated.
 * @return 0 if successful, 1 if a syntax error occurs.
 */
short handleDirective(string &str, const string &fName, size_t &lnNbr, WebConfigFile &config)
{
    static bool srvActive = false;
    static bool inLocation = false;
    static Server srvTmp;
    static Location locTmp(srvTmp);

    vector<string> tokens = split(str);
    if (tokens.empty())
        return (0);

    if ((tokens.size() == 1 && tokens[0] == "server{") ||
        (tokens.size() == 2 && tokens[0] == "server" && tokens[1] == "{"))
    {
        if (srvActive)
            return printError(str, fName, lnNbr);
        srvActive = true;
        srvTmp = Server();
        return (0);
    }

    if ((tokens.size() == 1 && tokens[0] == "location{") ||
        (tokens.size() == 2 && tokens[0] == "location" && tokens[1] == "{"))
    {
        if (!srvActive || inLocation)
            return printError(str, fName, lnNbr);
        inLocation = true;
        locTmp = Location(srvTmp);
        return (0);
    }

    if (tokens[0] == "}")
    {
        if (inLocation)
        {
            if (locTmp.route == "")
                return printError(str, fName, lnNbr);
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
            return printError(str, fName, lnNbr);
        return (0);
    }

    if (inLocation)
        return handleLocation(str, tokens, locTmp, fName, lnNbr);
    else if (srvActive)
        return handleServer(str, tokens, srvTmp, fName, lnNbr);
    else
        return printError(str, fName, lnNbr);

    return (0);
}

/**
 * @brief Constructs a WebConfigFile by parsing a configuration file.
 *
 * This constructor reads the configuration file line by line, removes
 * comments, trims spaces, and delegates the processing of each line
 * to handleDirective. It populates the internal vector of Server
 * objects based on the configuration.
 *
 * Behavior:
 * - Exits the program with an error if the file cannot be opened.
 * - Exits the program with an error if the configuration is empty.
 * - Exits the program with an error if any line fails to parse.
 *
 * @param fName The path to the configuration file to read.
 */
WebConfigFile::WebConfigFile(const string &fName)
{
    ifstream inputFile(fName.c_str());
    if (!inputFile.is_open())
    {
        cerr << "Error: Cannot open config file " << fName << endl;
        exit(1);
    }
    string currentLine;
    size_t lnNbr = 0;
    while (getline(inputFile, currentLine))
    {
        ++lnNbr;
        currentLine = removeComment(currentLine);
        if (currentLine.empty())
            continue;

        if (handleDirective(currentLine, fName, lnNbr, *this))
            exit(1);
    }
    if (lnNbr == 0)
    {
        cerr << "Error: Configuration file is empty" << endl;
        exit(1);
    }
    inputFile.close();
}
