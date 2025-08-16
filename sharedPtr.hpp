#pragma once
#ifndef SHAREDPTR_HPP
#define SHAREDPTR_HPP

#include <iostream>

template <typename _T>
class sharedPtr {
    long    *_count;
    _T      *_ptr;
    void    (*_deleter)(_T*);

public:

    /// @brief Creates an empty shared pointer (NULL)
    sharedPtr():
        _count(NULL),
        _ptr(NULL),
        _deleter(NULL) {}

    /// @brief Creates a shared pointer that takes ownership of ptr
    /// @param ptr Raw pointer to manage
    explicit sharedPtr(_T *ptr):
        _count(new long(1)),
        _ptr(ptr),
        _deleter(NULL) {}

    /// @brief Creates a shared pointer with a custom deleter
    /// @param ptr Raw pointer to manage
    /// @param deleter Function to use for deletion instead of delete
    explicit sharedPtr(_T *ptr, void (*deleter)(_T*)):
        _count(new long(1)),
        _ptr(ptr),
        _deleter(deleter) {}

    /// @brief Copy constructor (shares ownership)
    /// @param copy Shared pointer to copy from
    sharedPtr<_T>(const sharedPtr<_T> &copy)
    { *this = copy; }

    /// @brief Destructor (decrements reference count)
    ~sharedPtr()
    { _release(); }

    /// @brief Assignment operator (transfers ownership)
    /// @param copy Shared pointer to assign from
    /// @return Reference to this shared pointer
    sharedPtr<_T>& operator=(const sharedPtr<_T> &copy) {
        if (this == &copy) return *this;
        _release();
        _ptr = copy._ptr;
        _count = copy._count;
        _deleter = copy._deleter;
        if (_count) (*_count)++;
        return *this;
    }

    /// @brief Gets the current reference count
    /// @return Number of shared_ptr instances managing the object
    long use_count(void) const {
        if (!_count) return 0;
        return *_count;
    }

    /// @brief Gets the managed pointer
    /// @return Raw pointer being managed (does not transfer ownership)
    _T* get(void) const { return _ptr; }

    /// @brief Swaps the managed objects with another shared_ptr
    /// @param other Shared pointer to swap with
    void swap(sharedPtr<_T> &other) {
        if (this == &other) return;
        std::swap(_count, other._count);
        std::swap(_ptr, other._ptr);
        std::swap(_deleter, other._deleter);
    }

    /// @brief Replaces the managed object
    /// @param ptr New raw pointer to manage (NULL releases current object)
    void reset(_T* ptr = NULL) {
        if (!ptr) return _release();
        sharedPtr<_T> tmp(ptr);
        swap(tmp);
    }

    /// @brief Dereference operator
    /// @return Reference to the managed object
    /// @warning Undefined behavior if pointer is null
    _T& operator*() const { return *_ptr; }

    /// @brief Member access operator
    /// @return Pointer to the managed object
    /// @warning Undefined behavior if pointer is null
    _T* operator->() const { return _ptr; }

    /// @brief Boolean conversion operator
    /// @return true if managing an object, false otherwise
    operator bool() const { return _ptr != NULL; }

private:
    /// @internal
    /// @brief Releases ownership and decrements reference count
    void _release(void) {
        if (!_count) return;
        if (--(*_count) != 0) return;
        if (_deleter)
            _deleter(_ptr);
        else
            delete _ptr;
        delete _count;
        _count = NULL;
        _ptr = NULL;
    }
};

#endif
