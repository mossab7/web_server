#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <string>
#include <map>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>
#include "Pipe.hpp"
#include "HTTPParser.hpp"
#include "Epoll.hpp"

#define BUFFER_SIZE 4096

class CGIHandler
{
    private:
        std::string _scriptPath;
        std::vector<char *> _env;
        std::vector<char *> _argv;
        Pipe _inputPipe;
        Pipe _outputPipe;
        pid_t _pid;
        bool _isRunning;

        CGIHandler(const CGIHandler &other);
        CGIHandler &operator=(const CGIHandler &other);
        void initEnv(HTTPParser &parser);
        void initArgv();

    public:
        CGIHandler(const std::string &scriptPath, HTTPParser &parser);
        ~CGIHandler();

        void start();
        bool isRunning() const;
        void feedInput(const char *data, size_t size);
        void kill();
};

void CGIHandler::initArgv()
{
    //implement look up for interpreter if needed
    _argv.clear();
    _argv.push_back(const_cast<char*>(_scriptPath.c_str()));
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

CGIHandler::CGIHandler(const std::string &scriptPath, HTTPParser &parser)
    : _scriptPath(scriptPath), _pid(-1), _isRunning(false)
{
    initEnv(parser);
}

CGIHandler::~CGIHandler()
{
    if (_isRunning)
        kill();
    // Cleanup allocated environment strings
    for (size_t i = 0; i < _env.size() - 1; ++i)
        delete[] _env[i];
    _env.clear();
}

void CGIHandler::start()
{
    if (_isRunning)
        return;

    _pid = fork();
    if (_pid < 0)
    {
        throw std::runtime_error("Failed to fork process for CGI");
    }
    else if (_pid == 0)
    {
        // Child process
        dup2(_inputPipe.read_fd(), STDIN_FILENO);
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
        _outputPipe.set_non_blocking();
        _inputPipe.set_non_blocking();
        Epoll::getInstance().add_fd(_outputPipe.read_fd(), EPOLLIN);
        Epoll::getInstance().add_fd(_inputPipe.write_fd(), EPOLLOUT);
        _inputPipe.closeRead();
        _outputPipe.closeWrite();
        _isRunning = true;
    }
}

bool CGIHandler::isRunning() const
{
    return _isRunning;
}

void CGIHandler::feedInput(const char *data, size_t size)
{
    if (!_isRunning || !data || size == 0)
        return;

    _inputPipe.write(data, size);
}

void CGIHandler::kill()
{
    if (!_isRunning)
        return;

    ::kill(_pid, SIGKILL);
    int status;
    waitpid(_pid, &status, 0);
    _isRunning = false;

    // Cleanup allocated environment strings
    for (size_t i = 0; i < _env.size() - 1; ++i)
        delete[] _env[i];
    _env.clear();
}

#endif //CGI_HANDLER_HPP