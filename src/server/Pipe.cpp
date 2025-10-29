#include "Pipe.hpp"
#include "Logger.hpp"
std::string intToString(int value);
Pipe::Pipe()
{
    fd[0] = -1;
    fd[1] = -1;
    Logger logger;
    logger.debug("pipe constructor is called");
}

void Pipe::open()
{
    Logger logger;
    logger.info("Creating pipe");
    if (pipe(fd) == -1) 
    {
        logger.debug("pipe() syscall failed");
        throw std::runtime_error("Failed to create pipe");
    }
    logger.debug("pipe created with read fd: " + intToString(fd[0]) + ", write fd: " + intToString(fd[1]));
}   

Pipe::~Pipe()
{
   close();
}

int Pipe::read_fd() const
{
    return fd[0];
}

int Pipe::write_fd() const
{
    return fd[1];
}

void Pipe::close()
{
    closeRead();
    closeWrite();
}


int Pipe::read(char *buffer, size_t size)
{
    ssize_t bytesRead = ::read(fd[0], buffer, size);
    if (bytesRead < 0)  
    {
        throw std::runtime_error("Failed to read from pipe");
    }
    return bytesRead;
}

int Pipe::write(const char *data, size_t size)
{
    ssize_t bytesWritten = ::write(fd[1], data, size);
    if (bytesWritten < 0) 
    {
        throw std::runtime_error("Failed to write to pipe");
    }
    return bytesWritten;
}

void Pipe::closeRead()
{
    if (fd[0] != -1)
    {
        ::close(fd[0]);
        fd[0] = -1;
    }
}

void Pipe::closeWrite()
{
    if (fd[1] != -1)
    {
        ::close(fd[1]);
        fd[1] = -1;
    }
}
#include "Logger.hpp"
std::string intToString(int value);
void Pipe::set_non_blocking()
{
    int flags;

    Logger logger;
    logger.info("Setting pipe fds to non-blocking mode");
    logger.debug("Read fd: " + intToString(fd[0]) + ", Write fd: " + intToString(fd[1]));

    if (fd[0] != -1)
    {
        flags = fcntl(fd[0], F_GETFL, 0);
        if (flags == -1)
            throw std::runtime_error("Failed to get pipe read flags");
        if (fcntl(fd[0], F_SETFL, flags | O_NONBLOCK) == -1)
            throw std::runtime_error("Failed to set pipe read non-blocking");
    }

    if (fd[1] != -1)
    {
        flags = fcntl(fd[1], F_GETFL, 0);
        if (flags == -1)
            throw std::runtime_error("Failed to get pipe write flags");
        if (fcntl(fd[1], F_SETFL, flags | O_NONBLOCK) == -1)
            throw std::runtime_error("Failed to set pipe write non-blocking");
    }
}