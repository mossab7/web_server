#include "RingBuffer.hpp"
#include <iostream>
#include <algorithm>

RingBuffer::RingBuffer(size_t size):
    _head(0),
    _tail(0),
    _capacity(size),
    _size(0)
{
    _buff.reserve(size);
}

size_t  RingBuffer::getCapacity() const { return _capacity; }

size_t  RingBuffer::getSize() const { return _size; }
#include "Logger.hpp"
void    RingBuffer::clear()
{
    //::bzero(_buff.data(), _capacity);
    //Logger logger;
    //logger.debug(_buff.data());
    _head = 0;
    _tail = 0;
    _size = 0;
}

size_t  RingBuffer::write(const char *buff, size_t size)
{
    if (!size) return 0;

    size_t offset = 0;
    size_t toWrite = size;
    if (toWrite > _capacity) // just in case size > _capacity
    {
        offset = size - _capacity;
        toWrite = _capacity;
    }

    if (_head + toWrite <= _capacity)
        std::memcpy(&_buff[_head], buff + offset, toWrite);
    else
    {
        size_t chunk = _capacity - _head;
        std::memcpy(&_buff[_head], buff + offset, chunk);
        std::memcpy(&_buff[0], buff + offset + chunk, toWrite - chunk);
    }

    _head = (_head + toWrite) % _capacity;
    _size += toWrite;
    if (_size > _capacity)
    {
        size_t overflow = _size - _capacity;
        _tail = (_tail + overflow) % _capacity;
        _size = _capacity;
    }
    return size;
}
size_t  RingBuffer::read(char *buff, size_t size)
{
    // read only what's available
    size_t toRead = (size < _size) ? size : _size;
    if (!toRead) return 0;

    if (_tail + toRead <= _capacity)
        std::memcpy(buff, &_buff[_tail], toRead);
    else
    {
        size_t chunk = _capacity - _tail;
        std::memcpy(buff, &_buff[_tail], chunk);
        std::memcpy(buff + chunk, &_buff[0], toRead - chunk);
    }

    _tail = (_tail + toRead) % _capacity;
    _size -= toRead;
    return toRead;
}
size_t  RingBuffer::peek(char *buff, size_t size)
{
    // read only what's available
    size_t toRead = (size < _size) ? size : _size;
    if (!toRead) return 0;

    if (_tail + toRead <= _capacity)
        std::memcpy(buff, &_buff[_tail], toRead);
    else
    {
        size_t chunk = _capacity - _tail;
        std::memcpy(buff, &_buff[_tail], chunk);
        std::memcpy(buff + chunk, &_buff[0], toRead - chunk);
    }

    return toRead;
}

bool    RingBuffer::isFull() const { return _size == _capacity; }
bool    RingBuffer::isEmpty() const { return !_size; }

void    RingBuffer::advanceRead(size_t size)
{
    _tail = (_tail + size) % _capacity;
    _size -= size <= _capacity ? size : _size;
}

void    RingBuffer::advanceWrite(size_t size)
{
    _head = (_head + size) % _capacity;
    _size += size;
    if (_size > _capacity)
    {
        size_t overflow = _size - _capacity;
        _tail = (_tail + overflow) % _capacity;
        _size = _capacity;
    }
}

// int main()
// {
//     RingBuffer buff(5);

//     char test[] = "1234567"; // size = 7
//     buff.write(test, strlen(test));

//     char test1[] = "xx"; // size = 20
//     buff.write(test1, strlen(test1));

//     char holder[10];
//     size_t read_count = buff.read(holder, 10);

//     std::cout.write(holder, read_count);
//     std::cout << std::endl;

//     return 0;
// }
