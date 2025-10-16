#include "Pipe.hpp"

Pipe::Pipe()
{
    if (pipe(fd) == -1) 
    {
        throw std::runtime_error("Failed to create pipe");
    }
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
        ::close(fd[0]);
}

void Pipe::closeWrite()
{
    if (fd[1] != -1)
        ::close(fd[1]);
}


void Pipe::set_non_blocking()
{
    int flags;

    flags = fcntl(fd[0], F_GETFL, 0);
    if (flags == -1)
        throw std::runtime_error("Failed to get pipe read flags");
    if (fcntl(fd[0], F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::runtime_error("Failed to set pipe read non-blocking");

    flags = fcntl(fd[1], F_GETFL, 0);
    if (flags == -1)
        throw std::runtime_error("Failed to get pipe write flags");
    if (fcntl(fd[1], F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::runtime_error("Failed to set pipe write non-blocking");
}