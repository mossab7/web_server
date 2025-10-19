#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "EventHandler.hpp"
#include <string>
#include <map>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>
#include <fstream>
#include "Pipe.hpp"
#include "HTTPParser.hpp"
#include "Epoll.hpp"
#include "RequestHandler.hpp"
#include "FdManager.hpp"
#include "Routing.hpp"
#include <ctype.h>

#define BUFFER_SIZE 4096


class CGIHandler : public EventHandler
{
    private:
        std::string _scriptPath;
        std::vector<char *> _env;
        std::vector<char *> _argv;
        Pipe _inputPipe;
        Pipe _outputPipe;
        pid_t _pid;
        HTTPParser &_Reqparser;
		HTTPParser _cgiParser;
		HTTPResponse &_response;
        bool _isRunning;
		bool  _needBody;

        void push_interpreter_if_needed();
        void initEnv(HTTPParser &parser);
        void initArgv();
    public:
        CGIHandler(HTTPParser &parser, ServerConfig &config, FdManager &fdm);
        ~CGIHandler() {}
        int get_fd();
        void start(const RouteMatch& match, bool needBody);
        void onEvent(uint32_t events);
        void onReadable();
        void onWritable();
        void onError();
        bool isRunning() const;
        void end();
};

void CGIHandler::onEvent(uint32_t events)
{
    if (IS_ERROR_EVENT(events))
    {
        onError();
        return;
    }
    if (IS_READ_EVENT(events))
        onReadable();
    if (IS_WRITE_EVENT(events))
        onWritable();
}

void CGIHandler::onReadable()
{
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead = _outputPipe.read(buffer, BUFFER_SIZE);
    if (bytesRead < 0)
    {
        // Handle read error
        onError();
        return;
    }
    else if (bytesRead == 0)
    {
        // EOF, CGI process has finished output
        _fd_manager.detachFd(_outputPipe.read_fd());
        _isRunning = false;
        return;
    }
    else
    {
        // Process the read data (e.g., append to response body)
        _cgiParser.addChunk(buffer, bytesRead);
		if (_cgiParser.isError())
			//handle error
		_response.feed(buffer, bytesRead);
        //todo write to the toWrite buffer
    }

}

void CGIHandler::onWritable()
{
    size_t size = _Reqparser.getBody().getCapacity() - _Reqparser.getBody().getSize();
    char buffer[size + 1];
    _Reqparser.getBody().read(buffer, size);
    _inputPipe.write(buffer, size);
}

void CGIHandler::onError()
{
    // design better error handling
}

std::map <std::string, std::string> initInterpreterMap()
{
    std::map <std::string, std::string> interpreterMap;
    // --- Python ---
    interpreterMap[".py"]   = "/usr/bin/python3";
    interpreterMap[".py2"]  = "/usr/bin/python2";
    interpreterMap[".py3"]  = "/usr/bin/python3";

    // --- Perl ---
    interpreterMap[".pl"]   = "/usr/bin/perl";
    interpreterMap[".cgi"]  = "/usr/bin/perl";   // legacy CGI often Perl

    // --- Ruby ---
    interpreterMap[".rb"]   = "/usr/bin/ruby";

    // --- PHP ---
    interpreterMap[".php"]  = "/usr/bin/php";

    // --- Shell / POSIX ---
    interpreterMap[".sh"]   = "/bin/sh";
    interpreterMap[".bash"] = "/bin/bash";
    interpreterMap[".ksh"]  = "/bin/ksh";
    interpreterMap[".csh"]  = "/bin/csh";
    interpreterMap[".tcsh"] = "/bin/tcsh";
    interpreterMap[".zsh"]  = "/bin/zsh";

    // --- Tcl ---
    interpreterMap[".tcl"]  = "/usr/bin/tclsh";

    // --- Lua ---
    interpreterMap[".lua"]  = "/usr/bin/lua";

    // --- JavaScript / Node.js ---
    interpreterMap[".js"]   = "/usr/bin/node";
    interpreterMap[".mjs"]  = "/usr/bin/node";
    interpreterMap[".cjs"]  = "/usr/bin/node";

    // --- awk / sed ---
    interpreterMap[".awk"]  = "/usr/bin/awk";
    interpreterMap[".sed"]  = "/bin/sed";

    // --- R language ---
    interpreterMap[".r"]    = "/usr/bin/Rscript";

    // --- Java ---
    interpreterMap[".java"] = "/usr/bin/java";       // needs compiled class
    interpreterMap[".jar"]  = "/usr/bin/java -jar";  // run JAR directly

    // --- Scala / Kotlin (JVM-based) ---
    interpreterMap[".scala"]  = "/usr/bin/scala";
    interpreterMap[".kt"]     = "/usr/bin/kotlinc";   // compile first
    interpreterMap[".kts"]    = "/usr/bin/kotlin";    // Kotlin script

    // --- Groovy ---
    interpreterMap[".groovy"] = "/usr/bin/groovy";

    // --- Haskell ---
    interpreterMap[".hs"]   = "/usr/bin/runhaskell";

    // --- Scheme / Lisp ---
    interpreterMap[".scm"]  = "/usr/bin/guile";
    interpreterMap[".ss"]   = "/usr/bin/guile";
    interpreterMap[".lisp"] = "/usr/bin/clisp";

    // --- OCaml ---
    interpreterMap[".ml"]   = "/usr/bin/ocaml";

    // --- Erlang / Elixir ---
    interpreterMap[".erl"]  = "/usr/bin/escript";
    interpreterMap[".exs"]  = "/usr/bin/elixir";

    // --- Julia ---
    interpreterMap[".jl"]   = "/usr/bin/julia";

    // --- Go (script mode with yaegi / go run) ---
    interpreterMap[".go"]   = "/usr/bin/go run";

    // --- Swift ---
    interpreterMap[".swift"] = "/usr/bin/swift";

    // --- Dart ---
    interpreterMap[".dart"]  = "/usr/bin/dart";

    // --- PowerShell (cross-platform) ---
    interpreterMap[".ps1"]   = "/usr/bin/pwsh";      // PowerShell Core
    // Windows may use: "C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe"

    // --- Miscellaneous ---
    interpreterMap[".raku"]  = "/usr/bin/raku";   // formerly Perl 6
    interpreterMap[".cr"]    = "/usr/bin/crystal";
    interpreterMap[".nim"]   = "/usr/bin/nim";    // usually compiles to binary

    return interpreterMap;
}

void CGIHandler::push_interpreter_if_needed()
{
    std::string interpreter;
    std::string extension;
    std::map <std::string, std::string> interpreterMap;

    if (access(_scriptPath.c_str(), X_OK) != 0)
        return ; // not runnable at all

    // 2. Read first two bytes
    std::ifstream file(_scriptPath, std::ios::binary);
    if (!file) return;

    char header[2];
    file.read(header, 2);

    // Case A: shebang present -> kernel handles it
    if (header[0] == '#' && header[1] == '!') {
        interpreter.clear();  // no interpreter needed from map
        return  ;
    }

    // Case B: ELF binary -> no interpreter
    if ((unsigned char)header[0] == 0x7f && header[1] == 'E') {
        interpreter.clear();
        return ;
    }

    size_t dotPos = _scriptPath.rfind('.');
    if (dotPos != std::string::npos)
    {
        extension = _scriptPath.substr(dotPos);
        interpreterMap = initInterpreterMap();
        if (interpreterMap.find(extension) != interpreterMap.end())
        {
            interpreter = interpreterMap[extension];
            _argv.push_back(const_cast<char *>(interpreter.c_str()));
        }
        return;
    }
    throw std::runtime_error(_scriptPath + ": Unknown script type or no interpreter found");
    return;
}

void CGIHandler::initArgv()
{
    //implement look up for interpreter if needed
    _argv.clear();
    try
    {
        push_interpreter_if_needed();
    }
    catch(const std::exception& e)
    {
        throw;
    }

    _argv.push_back(const_cast<char *>(_scriptPath.c_str()));
    _argv.push_back(NULL); // Null-terminate for execve
}

void CGIHandler::initEnv(HTTPParser &parser)
{
    std::vector<std::string> envStrings;

    envStrings.push_back("GATEWAY_INTERFACE=CGI/1.1");
    envStrings.push_back("SERVER_PROTOCOL=" + parser.getVers());
    envStrings.push_back("REQUEST_METHOD=" + parser.getMethod());
    envStrings.push_back("REQUEST_URI=" + parser.getUri());

    // Add headers as HTTP_ environment variables
    strmap headers = parser.getHeaders();
    for (strmap::const_iterator it = headers.begin(); it != headers.end(); ++it)
    {
        std::string key = it->first;
        std::transform(key.begin(), key.end(), key.begin(), ::toupper);
        std::replace(key.begin(), key.end(), '-', '_');
        envStrings.push_back("HTTP_" + key + "=" + it->second);
    }

    // Content-Length and Content-Type
    if (headers.find("content-length") != headers.end())
        envStrings.push_back("CONTENT_LENGTH=" + headers["content-length"]);
    if (headers.find("content-type") != headers.end())
        envStrings.push_back("CONTENT_TYPE=" + headers["content-type"]);

    // Allocate and store char* pointers
    _env.clear();
    for (size_t i = 0; i < envStrings.size(); ++i)
    {
        char* envCStr = new char[envStrings[i].size() + 1];
        std::strcpy(envCStr, envStrings[i].c_str());
        _env.push_back(envCStr);
    }
    _env.push_back(NULL); // Null-terminate for execve
}

CGIHandler::CGIHandler(HTTPParser &parser, ServerConfig &config, FdManager &fdm)
    :_pid(-1), _isRunning(false) , _Reqparser(parser), EventHandler(config, fdm)
{
}

CGIHandler::~CGIHandler()
{
    if (_isRunning)
        kill();
    // Cleanup allocated environment strings
    for (size_t i = 0; i < _env.size() - 1; ++i)
        delete[] _env[i];
    _env.clear();
    _fd_manager.detachFd(_outputPipe.read_fd());
    _fd_manager.detachFd(_inputPipe.write_fd());
}

void CGIHandler::start(const RouteMatch& match, bool needBody)
{
    if (_isRunning)
        return;
	_needBody = needBody;
    try
    {
        initArgv();
        initEnv(_Reqparser);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Failed to initialize CGI: " + std::string(e.what()));
    }
    _pid = fork();
    if (_pid < 0)
    {
        throw std::runtime_error("Failed to fork process for CGI");
    }
    else if (_pid == 0)
    {
        // Child process
        dup2(_inputPipe.read_fd(), STDIN_FILENO);
		if (_needBody)
        	dup2(_outputPipe.write_fd(), STDOUT_FILENO);

        _inputPipe.close();
        _outputPipe.close();

        if (execve(_scriptPath.c_str(), _argv.data(), _env.data()) == -1)
        {
            // Cleanup allocated environment strings
            for (size_t i = 0; i < _env.size() - 1; ++i)
                delete[] _env[i];
            exit(1); // Exit child process on failure
        }
    }
    else
    {
        // Parent process
		if (_needBody)
		{
        	_outputPipe.set_non_blocking();
			_fd_manager.add(_outputPipe.read_fd(), this, EPOLLIN);
		}
        _inputPipe.set_non_blocking();
        _fd_manager.add(_inputPipe.write_fd(), this, EPOLLOUT);
        _inputPipe.closeRead();
        _outputPipe.closeWrite();
        _isRunning = true;
    }
}

bool CGIHandler::isRunning() const
{
    return _isRunning;
}


void CGIHandler::end()
{
    if (!_isRunning)
        return;

    ::kill(_pid, SIGKILL);
    int status;
    waitpid(_pid, &status, 0);
    _isRunning = false;

    for (size_t i = 0; i < _env.size() - 1; ++i)
        delete[] _env[i];
    _env.clear();
}


#endif //CGI_HANDLER_HPP
