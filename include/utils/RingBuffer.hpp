#ifndef WEBSERV_RINGBUFF_HPP
#define WEBSERV_RINGBUFF_HPP

#include <vector>
#include <cstring>

class RingBuffer
{
    std::vector<char> _buff;
    size_t            _head;     // Next write position
    size_t            _tail;     // Next read position
    size_t            _capacity; // Total capacity
    size_t            _size;     // Current amount of data stored

public:
    explicit RingBuffer(size_t size);

    size_t  write(const char* buff, size_t size); // write to buffer, returns bytes written
    size_t  read(char* buff, size_t size);  // read from _buff to buff, returns bytes read
    size_t  peek(char* buff, size_t size);  // read without advancing the pointers

    size_t  getCapacity(void) const;
    size_t  getSize(void) const;            // Get current data size

    void    clear(void);
};

#endif