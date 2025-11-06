#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "EventHandler.hpp"
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>
#include <fstream>
#include <signal.h>
#include "Pipe.hpp"
#include "HTTPParser.hpp"
#include "Epoll.hpp"
#include "Response.hpp"
#include "FdManager.hpp"
#include "Routing.hpp"
#include <ctype.h>
#include "../utils/Logger.hpp"
#include <time.h>
#define BUFFER_SIZE 4096

// Helper function to convert int to string
std::string intToString(int value);

class CGIHandler : public EventHandler
{
private:
	std::string _scriptPath;
	std::string _interpreterPath; // Store copy for argv
	std::vector<char *> _env;
	std::vector<char *> _argv;
	Pipe _inputPipe;
	Pipe _outputPipe;
	pid_t _pid;
	int status;
	HTTPParser &_Reqparser;
	HTTPParser _cgiParser;
	HTTPResponse &_response;
	RouteMatch _match;
	bool _isRunning;
	bool _needBody;

	bool _ShouldAddSLine;

	time_t expires_at;

	//void init_(HTTPParser &parser, RouteMatch const &match);
	void initEnv(HTTPParser &parser);
	void initArgv(RouteMatch const &match);

public:
	CGIHandler(HTTPParser &parser, HTTPResponse &response, ServerConfig &config, FdManager &fdm);
	~CGIHandler();
	int get_fd();
	int getStatus();
	void start(const RouteMatch &match);
	void destroy();
	void onEvent(uint32_t events);
	void onReadable();
	void onWritable();
	void onError();
	void onTimeout();
	bool isRunning() const;
	void end();
	void reset();
};

#endif // CGI_HANDLER_HPP
