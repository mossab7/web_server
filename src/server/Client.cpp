#include "Client.hpp"

std::string intToString(int value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

Client::Client(int socket_fd, ServerConfig &config, FdManager &fdm) :
    EventHandler(config, fdm),
    _socket(socket_fd),
    _handler(config),
    _strFD(intToString(socket_fd)),
    _state(ST_READING)
{
    _socket.set_non_blocking();
}
Client::~Client() 
{ 
    Logger logger;
    logger.info("Client destructor called for fd: " + _strFD);
}

int Client::get_fd() const { return _socket.get_fd(); }



/*--------------------------------------------------------*/
void Client::onEvent(uint32_t events)
{
    if (IS_ERROR_EVENT(events)) {
        onError();
        return;
    }
    if (IS_READ_EVENT(events))
        onReadable();
    if (IS_WRITE_EVENT(events))
        onWritable();
}

void    Client::onError()
{
    logger.error("Error event on client fd: " + _strFD);
    _fd_manager.remove(get_fd());
}
void    Client::onReadable()
{
    _readData();
    switch (_state)
    {
    case ST_READING     : break;
    case ST_PROCESSING  : _processRequest(); break;
    case ST_PARSEERROR  : _processError(); break;
    case ST_ERROR       : _processError(); break;
    case ST_CLOSED      : _closeConnection(); break;
    default: break;
    }
}
void    Client::onWritable()
{
    _sendData();
    switch (_state)
    {
    case ST_SENDING     : break;
    case ST_ERROR       : _processError(); break;
    case ST_SENDCOMPLETE:
        if (_keepAlive) reset();
        else _state = ST_CLOSED;
        break;
    case ST_CLOSED: _closeConnection(); break;
    default: break;
    }
}
/*--------------------------------------------------------*/

bool Client::_readData()
{
    if (_state != ST_READING)
        return false;

    ssize_t size = _socket.recv(_readBuff, BUFF_SIZE - 1, 0);
    if (size < 0)
    {
        logger.error("Error on client fd: " + _strFD);
        _state = ST_ERROR;
        return false;
    }
    if (size == 0)
    {
        logger.warning("Connection closed on fd: " + _strFD);
        _state = ST_CLOSED;
        return false;
    }
    _readBuff[size] = '\0';
    _handler.feed(_readBuff, size);
    
    if (_handler.isError())
    {
        logger.debug("Parsing error on client fd: " + _strFD);
        _state = ST_PARSEERROR;
        return false;
    }
    if (_handler.isReqComplete())
    {
        _state = ST_PROCESSING;
        return false;
    }

    return true;
}
bool Client::_sendData()
{
    ssize_t toSend = _handler.readNextChunk(_sendBuff, BUFF_SIZE);
    if (toSend < 0)
    {
        logger.error("Error on Client fd: " + _strFD);
        _state = ST_ERROR;
        return false;
    }
    if (toSend == 0)
    {
        logger.debug("Client send response complete fd: " + _strFD);
        _state = ST_SENDCOMPLETE;
        return false;
    }
    
    ssize_t sent = _socket.send(_sendBuff, toSend, 0);
    if (sent < 0)
    {
        logger.error("Can't send data on client fd: " + _strFD);
        _state = ST_ERROR;
        return false;
    }
    if (sent < toSend)
    {
        logger.warning("Partial send on client fd: " + _strFD);
        return true;
    }
    
    if (_handler.isResComplete())
    {
        logger.debug("Sending response complete on client fd: " + _strFD);
        _state = ST_SENDCOMPLETE;
        return false;
    }
    
    return true;
}

void    Client::_closeConnection()
{
    // _response.closeFile();
    _fd_manager.remove(get_fd());
}

void    Client::reset()
{
    _handler.reset();
    _state = ST_READING;
    _fd_manager.modify(this, READ_EVENT);
}

void    Client::_processError()
{
    if (_state == ST_ERROR)
    {
        logger.error("Error on client fd: " + _strFD);
        _closeConnection();
        return;
    }
    logger.debug("Parsing error on client fd: " + _strFD);
    // todo: send status code '400 bad request'
    // for now I am just going to ignore it
}
void Client::_processRequest()
{
    _keepAlive = _shouldKeepAlive();
    _handler.processRequest();
    _state = ST_SENDING;
    _fd_manager.modify(this, WRITE_EVENT);
}

bool    Client::_shouldKeepAlive()
{
    return _handler.keepAlive();
}
