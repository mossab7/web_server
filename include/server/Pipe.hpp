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
    int read(char *buffer, size_t size);
    int write(const char *data, size_t size);
    int read_fd() const;
    int write_fd() const;
    void set_non_blocking();
    void closeRead();
    void closeWrite();
    void open();
    void close();
};

#endif // PIPE_HPP