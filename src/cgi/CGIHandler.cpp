#include "CGIHandler.hpp"

void CGIHandler::onEvent(uint32_t events)
{
	Logger logger;
	_updateExpiresAt(time(NULL) + _match.location->cgi_timeout);
	if (IS_ERROR_EVENT(events))
	{
		onError();
		return;
	}
	if (IS_READ_EVENT(events))
	{
		onReadable();
	}
	if (IS_WRITE_EVENT(events))
	{
		onWritable();
	}
	if (IS_TIMEOUT_EVENT(events))
	{
		onTimeout();
	}
}

void CGIHandler::onReadable()
{
	Logger logger;
	
	char buffer[BUFFER_SIZE];
	ssize_t bytesRead = _outputPipe.read(buffer, BUFFER_SIZE);

	if (bytesRead < 0)
	{
		logger.error("CGI read error");
		onError();
		return;
	}

	if (bytesRead == 0)
	{
		_fd_manager.detachFd(_outputPipe.read_fd());
		_outputPipe.closeRead();

		if (_cgiParser.getState() < BODY)
		{
			logger.error("CGI output incomplete - no body received");
			status = 502;
			_isRunning = false;
			onError();
			return;
		}

		_response.feedRAW("");
		_isRunning = false;
		return;
	}

	_cgiParser.addChunk(buffer, bytesRead);

	if (_cgiParser.isError())
	{
		logger.error("CGI response parsing error");
		status = 502;
		onError();
		return;
	}


	if (_ShouldAddSLine && _cgiParser.getState() >= BODY)
	{
		
		int statusCode = 200;
		std::string cgiStatus = _cgiParser.getHeader("status");
		if (!cgiStatus.empty())
		{
			statusCode = ft_atoi<int>(cgiStatus.c_str());
			if (statusCode == 0)
				statusCode = 200;
		}
		
		_response.startLine(statusCode);
		
		strmap& headers = _cgiParser.getHeaders();
		for (strmap::iterator it = headers.begin(); it != headers.end(); ++it)
		{
			std::string key = it->first;
			std::transform(key.begin(), key.end(), key.begin(), ::tolower);
			
			if (key == "status")
				continue;
			
			_response.addHeader(it->first, it->second);
		}
		
		_response.addHeader("Transfer-Encoding", "chunked");
		
		_response.endHeaders();
		
		_ShouldAddSLine = false;
		
	}

	if (_cgiParser.getState() >= BODY)
	{
		RingBuffer& body = _cgiParser.getBody();
		size_t bodySize = body.read(buffer, sizeof(buffer));
		
		if (bodySize > 0)
		{
			_response.feedRAW(buffer, bodySize);
		}
	}
}

void CGIHandler::onWritable()
{
	if (!_needBody)
	{
		_fd_manager.detachFd(_inputPipe.write_fd());
		_inputPipe.closeWrite();
		return;
	}

	RingBuffer& body = _Reqparser.getBody();
	size_t available = body.getSize();

	if (available == 0)
	{
		_fd_manager.detachFd(_inputPipe.write_fd());
		_inputPipe.closeWrite();
		return;
	}

	char buffer[BUFFER_SIZE];
	size_t toWrite = (available > BUFFER_SIZE) ? BUFFER_SIZE : available;
	size_t bytesRead = body.read(buffer, toWrite);

	if (bytesRead > 0)
	{
		ssize_t bytesWritten = _inputPipe.write(buffer, bytesRead);
		if (bytesWritten < 0)
		{
			Logger logger;
			logger.error("CGI write error");
			onError();
			return;
		}
	}

	if (body.getSize() == 0)
	{
		_fd_manager.detachFd(_inputPipe.write_fd());
		_inputPipe.closeWrite();
	}
}

void CGIHandler::onError()
{
	Logger logger;

	onReadable(); 
	if (_inputPipe.write_fd() != -1)
	{
		_fd_manager.detachFd(_inputPipe.write_fd());
		_inputPipe.closeWrite();
	}
	int waitStatus = 0;
	pid_t result = waitpid(_pid, &waitStatus, WNOHANG);

	if (result == 0)
	{
		logger.warning("CGI process still running, sending SIGKILL");
		::kill(_pid, SIGKILL);
		waitpid(_pid, &waitStatus, 0);
	}
	else if (result > 0)
	{
		if (WIFEXITED(waitStatus))
		{
			int exitStatus = WEXITSTATUS(waitStatus);
			if (exitStatus != 0)
			{
				status = 502;
			}
		}
		else if (WIFSIGNALED(waitStatus))
		{
			int signal = WTERMSIG(waitStatus);
			logger.error("CGI process terminated by signal: " + intToString(signal));
			status = 502;
		}
	}
}

void CGIHandler::initArgv(RouteMatch const &match)
{
	_argv.clear();

	if (!match.scriptInterpreter.empty())
	{
		_interpreterPath = match.scriptInterpreter;
		_argv.push_back(const_cast<char *>(_interpreterPath.c_str()));
	}
	Logger logger;
	_scriptPath = match.scriptPath;
	_argv.push_back(const_cast<char *>(_scriptPath.c_str()));
	_argv.push_back(NULL);
}

void CGIHandler::initEnv(HTTPParser &parser)
{
	std::vector<std::string> envStrings;

	envStrings.push_back("GATEWAY_INTERFACE=CGI/1.1");
	envStrings.push_back("SERVER_PROTOCOL=" + parser.getVers());
	envStrings.push_back("REQUEST_METHOD=" + parser.getMethod());
	envStrings.push_back("SCRIPT_NAME=" + parser.getUri());
	envStrings.push_back("SCRIPT_FILENAME=" + _scriptPath);
	envStrings.push_back("QUERY_STRING=" + parser.getQuery());

	envStrings.push_back("SERVER_NAME=" + (_config.host.empty() ? "localhost" : _config.host));
	envStrings.push_back("SERVER_PORT=" + intToString(_config.port));
	envStrings.push_back("SERVER_SOFTWARE=WebServ/1.0");

	// Remote address (would need to be passed from connection context)
	// envStrings.push_back("REMOTE_ADDR=127.0.0.1");

	strmap headers = parser.getHeaders();

	if (headers.find("content-length") != headers.end())
		envStrings.push_back("CONTENT_LENGTH=" + headers["content-length"]);
	else
		envStrings.push_back("CONTENT_LENGTH=0");

	if (headers.find("content-type") != headers.end())
		envStrings.push_back("CONTENT_TYPE=" + headers["content-type"]);

	for (strmap::const_iterator it = headers.begin(); it != headers.end(); ++it)
	{
		if (it->first == "content-length" || it->first == "content-type")
			continue;

		std::string key = it->first;
		std::transform(key.begin(), key.end(), key.begin(), ::toupper);
		std::replace(key.begin(), key.end(), '-', '_');
		envStrings.push_back("HTTP_" + key + "=" + it->second);
	}

	_env.clear();
	for (size_t i = 0; i < envStrings.size(); ++i)
	{
		char *envCStr = new char[envStrings[i].size() + 1];
		std::strcpy(envCStr, envStrings[i].c_str());
		_env.push_back(envCStr);
	}
	_env.push_back(NULL); 
}

CGIHandler::CGIHandler(HTTPParser &parser, HTTPResponse &response, ServerConfig &config, FdManager &fdm)
: EventHandler(config, fdm, -1),
    _scriptPath(""),
    _inputPipe(),
    _outputPipe(),
    _pid(-1),
    status(0),
    _Reqparser(parser),
    _cgiParser(),
    _response(response),
    _isRunning(false),
    _needBody(false),
	_ShouldAddSLine(true)
{
	_cgiParser.setCGIMode(true); 
}

CGIHandler::~CGIHandler()
{
	end();
}

void CGIHandler::start(const RouteMatch &match, bool body_availelbe)
{
	if (_isRunning)
		return;


	_needBody = body_availelbe;
	_scriptPath = match.scriptPath;
	_match = match;
	try
	{
		initArgv(match);
		initEnv(_Reqparser);
		_inputPipe.open();
		_outputPipe.open();
	}
	catch (const std::exception &e)
	{
		throw std::runtime_error("Failed to initialize CGI: " + std::string(e.what()));
	}
	Logger logger;
	logger.debug("CGI script path: " + _scriptPath);
	logger.debug("CGI interpreter path: " + _interpreterPath);

	if (!_interpreterPath.empty())
	{
		if (access(_interpreterPath.c_str(), X_OK) == -1)
		{
			status = 500;
			end();
			return;
		}
		if (access(_scriptPath.c_str(), R_OK) == -1)
		{
			status = 500;
			end();
			return;
		}
	}
	else
	{
		if (access(_scriptPath.c_str(), X_OK) == -1)
		{
			status = 500;
			end();
			return;
		}
	}
	_pid = fork();
	if (_pid < 0)
	{
		throw std::runtime_error("Failed to fork process for CGI");
	}
	else if (_pid == 0)
	{
		if (dup2(_inputPipe.read_fd(), STDIN_FILENO) == -1)
		{
			std::perror("dup2 stdin");
			std::exit(1);
		}

		if (dup2(_outputPipe.write_fd(), STDOUT_FILENO) == -1)
		{
			std::perror("dup2 stdout");
			std::exit(1);
		}

		_inputPipe.close();
		_outputPipe.close();

		execve(_argv[0], _argv.data(), _env.data());

		std::perror("execve");
		std::exit(1);
	}
	else
	{
        try 
        {
            _inputPipe.set_non_blocking();
            _outputPipe.set_non_blocking();
        }
        catch (const std::exception &e) 
        {
            throw std::runtime_error("Failed to set non-blocking mode for CGI pipes: " + std::string(e.what()));
        }

		_inputPipe.closeRead();
		_outputPipe.closeWrite();

		RingBuffer body = _Reqparser.getBody();
		_expiresAt = time(NULL) + match.location->cgi_timeout;
		if (_needBody && body.getSize() > 0)
		{
			_fd_manager.add(_inputPipe.write_fd(), this, EPOLLOUT, false);
		}
		else
		{
			_inputPipe.closeWrite();
		}

		_fd_manager.add(_outputPipe.read_fd(), this, EPOLLIN);

		_isRunning = true;
		expires_at = time(NULL) + match.location->cgi_timeout;
		for (size_t i = 0; i < _env.size(); ++i)
		{
			delete[] _env[i];
		}
		_env.clear();
	}
}

int CGIHandler::get_fd()
{
	return -1;
}

bool CGIHandler::isRunning() const
{
	return _isRunning;
}

void CGIHandler::end()
{
	_fd_manager.remove(_inputPipe.write_fd());
	_fd_manager.remove(_outputPipe.read_fd());
	_inputPipe.close();
	_outputPipe.close();
	for (size_t i = 0; i < _env.size(); ++i)
	{
		if (_env[i] != NULL)
			delete[] _env[i];
	}
	_env.clear();
	if (!_isRunning)
		return;

	::kill(_pid, SIGKILL);

	int waitStatus;
	waitpid(_pid, &waitStatus, 0);

	_isRunning = false;
}

void CGIHandler::reset()
{
	end();

	_fd_manager.detachFd(_inputPipe.write_fd());
	_fd_manager.detachFd(_outputPipe.read_fd());
	_inputPipe.close();
	_outputPipe.close();

	for (size_t i = 0; i < _env.size(); ++i)
	{
		if (_env[i] != NULL)
			delete[] _env[i];
	}
	_env.clear();

	_argv.clear();

	_scriptPath.clear();
	_interpreterPath.clear();
	_pid = -1;
	status = 0;
	_isRunning = false;
	_needBody = false;
	_ShouldAddSLine = true;

	_cgiParser.reset();
}

int CGIHandler::getStatus()
{
	return (status);
}

void CGIHandler::destroy()
{
	//cgi is owned by the client class so cleanup happens when client is destroyed
}

void CGIHandler::onTimeout()
{
	Logger logger;
	logger.error("CGIHandler::onTimeout() called - CGI script timed out");
	end();
	status = 504;
	_response.feedRAW("", 0);
	_isRunning = false;
}
