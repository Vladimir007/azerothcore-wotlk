#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <memory>
#include <mutex>
#include <vector>

template <typename T>
class CircularBuffer
{
public:
    explicit CircularBuffer(const std::size_t size) : _buf(std::unique_ptr<T[]>(new T[size])), _max_size(size) {}

    void put(T item)
    {
        std::lock_guard lock(_mutex);
        _buf[_head] = item;
        if (_full)
            _tail = (_tail + 1) % _max_size;
        _head = (_head + 1) % _max_size;
        _full = _head == _tail;
    }

    [[nodiscard]] bool empty() const
    {
        // If head and tail are equal, we are empty
        return !_full && _head == _tail;
    }

    [[nodiscard]] bool full() const { return _full; }

    [[nodiscard]] std::size_t capacity() const { return _max_size; }

    [[nodiscard]] std::size_t size() const
    {
        std::size_t size = _max_size;

        if (!_full)
        {
            if (_head >= _tail)
                size = _head - _tail;
            else
                size += _head - _tail;
        }

        return size;
    }

    // The implementation of this function is simplified by the fact that _head will never be lower than _tail
    // when compared to the original implementation of this class
    std::vector<T> content()
    {
        std::lock_guard lock(_mutex);
        return std::vector<T>(_buf.get(), _buf.get() + size());
    }

    T peak_back()
    {
        std::lock_guard lock(_mutex);
        return empty() ? T() : _buf[_tail];
    }

private:
    std::mutex _mutex;
    std::unique_ptr<T[]> _buf;
    std::size_t _head = 0;
    std::size_t _tail = 0;
    const std::size_t _max_size;
    bool _full = false;
};
#endif
