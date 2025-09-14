#ifndef PIPE_HPP
#define PIPE_HPP

#include <unistd.h>
#include <stdexcept>
#include <fcntl.h>

class Pipe
{
    private:
        int fd[2];
        Pipe(const Pipe &other);
        Pipe &operator=(const Pipe &other);
    public:
        Pipe();
        ~Pipe();
        int read_fd() const;
        int write_fd() const;
        void close();
};

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
    ::close(fd[0]);
    ::close(fd[1]);
}


#endif //PIPE_HPP