#include "CGIHandler.hpp"

void CGIHandler::onEvent(uint32_t events)
{
	Logger logger;
	logger.debug("CGIHandler::onEvent called with events: " + intToString(events));
	_updateExpiresAt(time(NULL) + _match.location->cgi_timeout);
	if (IS_ERROR_EVENT(events))
	{
		logger.debug("CGIHandler: Error event received");
		onError();
		return;
	}
	if (IS_READ_EVENT(events))
	{
		logger.debug("CGIHandler: Read event received");
		onReadable();
	}
	if (IS_WRITE_EVENT(events))
	{
		logger.debug("CGIHandler: Write event received");
		onWritable();
	}
	if (IS_TIMEOUT_EVENT(events))
	{
		logger.debug("CGIHandler: Timeout event received");
		onTimeout();
	}
}

void CGIHandler::onReadable()
{
	Logger logger;
	logger.debug("CGIHandler::onReadable() called");
	
	char buffer[BUFFER_SIZE];
	ssize_t bytesRead = _outputPipe.read(buffer, BUFFER_SIZE);

	logger.debug("CGI read " + intToString(bytesRead) + " bytes");

	if (bytesRead < 0)
	{
		logger.error("CGI read error");
		onError();
		return;
	}

	if (bytesRead == 0)
	{
		logger.debug("CGIHandler::onReadable() - EOF reached");
		_fd_manager.detachFd(_outputPipe.read_fd());
		_outputPipe.closeRead();

		// Check if CGI output was valid
		if (_cgiParser.getState() < BODY)
		{
			logger.error("CGI output incomplete - no body received");
			status = 502;
			//_response._cgiComplete = true;
			_isRunning = false;
			onError();
			return;
		}

		// Send final chunk (0 size) to end chunked transfer
		_response.feedRAW("");
		//_response._cgiComplete = true;
		_isRunning = false;
		return;
	}

	// Parse CGI output
	_cgiParser.addChunk(buffer, bytesRead);

	if (_cgiParser.isError())
	{
		logger.error("CGI response parsing error");
		status = 502;
		onError();
		return;
	}

	//logger.debug("CGI output parsed, output buffer: " + std::string(buffer));

	// When headers are complete, send HTTP response headers
	if (_ShouldAddSLine && _cgiParser.getState() >= BODY)
	{
		logger.debug("CGI headers parsed, building HTTP response");
		
		// Get status code from CGI (default 200)
		int statusCode = 200;
		std::string cgiStatus = _cgiParser.getHeader("status");
		if (!cgiStatus.empty())
		{
			statusCode = ft_atoi<int>(cgiStatus.c_str());
			if (statusCode == 0)
				statusCode = 200;
		}
		
		// Start HTTP response
		_response.startLine(statusCode);
		
		// Copy all CGI headers to HTTP response (except Status)
		strmap& headers = _cgiParser.getHeaders();
		for (strmap::iterator it = headers.begin(); it != headers.end(); ++it)
		{
			std::string key = it->first;
			std::transform(key.begin(), key.end(), key.begin(), ::tolower);
			
			if (key == "status")
				continue; // Skip status header
			
			_response.addHeader(it->first, it->second);
		}
		
		// Add Transfer-Encoding for chunked response
		_response.addHeader("Transfer-Encoding", "chunked");
		
		// End headers section
		_response.endHeaders();
		
		_ShouldAddSLine = false;
		
		logger.debug("HTTP response headers done");
	}

	// Send body data as chunks (only if we're in BODY state)
	if (_cgiParser.getState() >= BODY)
	{
		// _response.feedRAW("this is a test");
		RingBuffer& body = _cgiParser.getBody();
		size_t bodySize = body.read(buffer, sizeof(buffer));
		
		if (bodySize > 0)
		{
			//logger.debug("Sending CGI body chunk: " + intToString(bodySize) + " bytes");
			//logger.warning("CGI body chunk data: " + std::string(buffer, bodySize));
			_response.feedRAW(buffer, bodySize);
		}
	}
}

void CGIHandler::onWritable()
{
	if (!_needBody)
	{
		// No body to send, close the input pipe write end
		_fd_manager.detachFd(_inputPipe.write_fd());
		_inputPipe.closeWrite();
		return;
	}

	RingBuffer& body = _Reqparser.getBody();
	size_t available = body.getSize();

	if (available == 0)
	{
		// All body data has been sent
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
			// Write error
			Logger logger;
			logger.error("CGI write error");
			onError();
			return;
		}
	}

	// Check if we've written all the body data
	if (body.getSize() == 0)
	{
		_fd_manager.detachFd(_inputPipe.write_fd());
		_inputPipe.closeWrite();
	}
}

void CGIHandler::onError()
{
	Logger logger;

	// if (_cgiParser.isComplete())
	// just in case of the whole pipe was consumed in a single go (fk this)
		
	//char buffer[BUFFER_SIZE];
	//ssize_t bytesRead = _outputPipe.read(buffer, BUFFER_SIZE);
	//// if (bytesRead > 0)
	// {
	// 	_cgiParser.addChunk(buffer,bytesRead);
	//     if (_cgiParser.getState() == ERROR)
	//     {
	//     		//handle error;
    //     }
	// 	// _response.feedRAW(buffer,bytesRead);
    // }
	{
		_response.feedRAW("", 0);
		//_response._cgiComplete = true;
		_isRunning = false;
	}
	// Clean up pipes
	if (_inputPipe.write_fd() != -1)
	{
		logger.debug("CGIHandler::onError() - Closing input pipe" + intToString(_inputPipe.write_fd()));
		_fd_manager.detachFd(_inputPipe.write_fd());
		_inputPipe.closeWrite();
	}
	if (_outputPipe.read_fd() != -1)
	{
		logger.debug("CGIHandler::onError() - Closing output pipe" + intToString(_outputPipe.read_fd()));
		_fd_manager.detachFd(_outputPipe.read_fd());
		_outputPipe.closeRead();
	}

	// Check process status
	int waitStatus = 0;
	pid_t result = waitpid(_pid, &waitStatus, WNOHANG);

	if (result == 0)
	{
		// Process still running, kill it
		logger.warning("CGI process still running, sending SIGKILL");
		::kill(_pid, SIGKILL);
		waitpid(_pid, &waitStatus, 0);
	}
	else if (result > 0)
	{
		// Process has exited
		if (WIFEXITED(waitStatus))
		{
			int exitStatus = WEXITSTATUS(waitStatus);
			logger.debug("CGI process exited with status: " + intToString(exitStatus));
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

	//status = 502;
	//_response._cgiComplete = true;
	_isRunning = false;
}

void CGIHandler::initArgv(RouteMatch const &match)
{
	_argv.clear();

	// Store copies of the strings so we have valid pointers
	if (!match.scriptInterpreter.empty())
	{
		_interpreterPath = match.scriptInterpreter;
		_argv.push_back(const_cast<char *>(_interpreterPath.c_str()));
	}
	Logger logger;
	logger.debug("CGI interpreter path set to: " + _interpreterPath);
	_scriptPath = match.scriptPath;
	_argv.push_back(const_cast<char *>(_scriptPath.c_str()));
	_argv.push_back(NULL); // Null-terminate for execve
}

void CGIHandler::initEnv(HTTPParser &parser)
{
	std::vector<std::string> envStrings;

	// Standard CGI environment variables
	envStrings.push_back("GATEWAY_INTERFACE=CGI/1.1");
	envStrings.push_back("SERVER_PROTOCOL=" + parser.getVers());
	envStrings.push_back("REQUEST_METHOD=" + parser.getMethod());
	envStrings.push_back("SCRIPT_NAME=" + parser.getUri());
	envStrings.push_back("SCRIPT_FILENAME=" + _scriptPath);
	envStrings.push_back("QUERY_STRING=" + parser.getQuery());

	// Server information (use defaults if not available from config)
	envStrings.push_back("SERVER_NAME=" + (_config.host.empty() ? "localhost" : _config.host));
	envStrings.push_back("SERVER_PORT=" + intToString(_config.port));
	envStrings.push_back("SERVER_SOFTWARE=WebServ/1.0");

	// Remote address (would need to be passed from connection context)
	// envStrings.push_back("REMOTE_ADDR=127.0.0.1");

	// Get headers
	strmap headers = parser.getHeaders();

	// Content-Length and Content-Type (special handling - don't need HTTP_ prefix)
	if (headers.find("content-length") != headers.end())
		envStrings.push_back("CONTENT_LENGTH=" + headers["content-length"]);
	else
		envStrings.push_back("CONTENT_LENGTH=0");

	if (headers.find("content-type") != headers.end())
		envStrings.push_back("CONTENT_TYPE=" + headers["content-type"]);

	// Adding other headers as HTTP_ environment variables
	for (strmap::const_iterator it = headers.begin(); it != headers.end(); ++it)
	{
		// Skipping Content-Length and Content-Type as they're already handled
		if (it->first == "content-length" || it->first == "content-type")
			continue;

		std::string key = it->first;
		std::transform(key.begin(), key.end(), key.begin(), ::toupper);
		std::replace(key.begin(), key.end(), '-', '_');
		envStrings.push_back("HTTP_" + key + "=" + it->second);
	}

	// Allocate and store char* pointers
	_env.clear();
	for (size_t i = 0; i < envStrings.size(); ++i)
	{
		char *envCStr = new char[envStrings[i].size() + 1];
		std::strcpy(envCStr, envStrings[i].c_str());
		_env.push_back(envCStr);
	}
	_env.push_back(NULL); // Null-terminate for execve
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
	Logger logger;
	logger.debug("CGIHandler destructor called");
}

void CGIHandler::start(const RouteMatch &match)
{
	if (_isRunning)
		return;

	// reset CGI handler state
	//reset();

	_needBody = (_Reqparser.getMethod() == "POST" || _Reqparser.getMethod() == "PUT");
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

	//checking permissions
	/*--------------------------------------------------------------------------------*/
	if (!_interpreterPath.empty())
	{
		if (access(_interpreterPath.c_str(), X_OK) == -1)
		{
			status = 500;
			end();
			return;
			//throw std::runtime_error("CGI interpreter not executable: " + _interpreterPath);
		}
		if (access(_scriptPath.c_str(), R_OK) == -1)
		{
			status = 500;
			end();
			return;
			//throw std::runtime_error("CGI script not readable: " + _scriptPath);
		}
	}
	else
	{
		if (access(_scriptPath.c_str(), X_OK) == -1)
		{
			status = 500;
			end();
			return;
			//throw std::runtime_error("CGI script not executable: " + _scriptPath);
		}
	}
	/*--------------------------------------------------------------------------------*/
	_pid = fork();
	if (_pid < 0)
	{
		throw std::runtime_error("Failed to fork process for CGI");
	}
	else if (_pid == 0)
	{
		// Child process
		// Redirect stdin to input pipe
		if (dup2(_inputPipe.read_fd(), STDIN_FILENO) == -1)
		{
			std::perror("dup2 stdin");
			std::exit(1);
		}

		// Redirect stdout to output pipe
		if (dup2(_outputPipe.write_fd(), STDOUT_FILENO) == -1)
		{
			std::perror("dup2 stdout");
			std::exit(1);
		}

		// Close all pipe file descriptors in child
		_inputPipe.close();
		_outputPipe.close();

		// Execute the CGI script
		execve(_argv[0], _argv.data(), _env.data());

		// If execve fails
		std::perror("execve");
		std::exit(1);
	}
	else
	{
		// Parent process
		// Set pipes to non-blocking mode BEFORE closing unused ends
        try 
        {
            _inputPipe.set_non_blocking();
            _outputPipe.set_non_blocking();
        }
        catch (const std::exception &e) 
        {
            throw std::runtime_error("Failed to set non-blocking mode for CGI pipes: " + std::string(e.what()));
        }

		// Close unused pipe ends
		_inputPipe.closeRead();
		_outputPipe.closeWrite();

		// Register pipes with epoll
		RingBuffer body = _Reqparser.getBody();
		_expiresAt = time(NULL) + match.location->cgi_timeout;
		if (_needBody && body.getSize() > 0)
		{
			_fd_manager.add(_inputPipe.write_fd(), this, EPOLLOUT, false);
		}
		else
		{
			// No body to send, close write end immediately
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
	// Return the file descriptor being monitored
	// This could be either the input (write) or output (read) pipe
	// depending on what's currently active
	if (_outputPipe.read_fd() != -1)
		return _outputPipe.read_fd();
	if (_inputPipe.write_fd() != -1)
		return _inputPipe.write_fd();
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
	// Clean up any remaining environment variables
	for (size_t i = 0; i < _env.size(); ++i)
	{
		if (_env[i] != NULL)
			delete[] _env[i];
	}
	_env.clear();
	if (!_isRunning)
		return;

	// Kill the child process
	::kill(_pid, SIGKILL);

	// Wait for the process to actually terminate (without WNOHANG)
	int waitStatus;
	waitpid(_pid, &waitStatus, 0);

	_isRunning = false;
}

void CGIHandler::reset()
{
	// End any running CGI process
	end();

	//detach and close pipes (close already checks if pipe is closed)
	_fd_manager.detachFd(_inputPipe.write_fd());
	_fd_manager.detachFd(_outputPipe.read_fd());
	_inputPipe.close();
	_outputPipe.close();

	// Clean up any remaining environment variables
	for (size_t i = 0; i < _env.size(); ++i)
	{
		if (_env[i] != NULL)
			delete[] _env[i];
	}
	_env.clear();

	// Clean up argv
	_argv.clear();

	// Reset state variables
	_scriptPath.clear();
	_interpreterPath.clear();
	_pid = -1;
	status = 0;
	_isRunning = false;
	_needBody = false;
	_ShouldAddSLine = true;

	// Reset the CGI parser for the next request
	_cgiParser.reset();

	// Note: _Reqparser and _response are references and should be
	// updated externally before calling start() again
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
