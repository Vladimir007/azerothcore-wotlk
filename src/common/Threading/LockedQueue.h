#ifndef LOCKED_QUEUE_H
#define LOCKED_QUEUE_H

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>

template <class T, typename StorageType = std::deque<T>>
class LockedQueue
{
    mutable std::mutex _lock; ///< Mutex to protect access to the queue
    std::atomic<bool> _canceled{false}; ///< Flag indicating if the queue is canceled
    StorageType _queue; ///< Storage container for the queue

public:
    LockedQueue() = default;
    virtual ~LockedQueue() = default;

    void add(const T& item)
    {
        std::lock_guard lock(_lock);
        _queue.push_back(std::move(item));
    }

    /// Gets the next item in the queue and removes it.
    bool next(T& result)
    {
        std::lock_guard lock(_lock);
        if (_queue.empty())
            return false;

        result = std::move(_queue.front());
        _queue.pop_front();
        return true;
    }

    /// Retrieves the next item from the queue if it satisfies the provided checker.
    template<class Checker>
    bool next(T& result, Checker& check)
    {
        std::lock_guard lock(_lock);
        if (_queue.empty())
            return false;

        result = std::move(_queue.front());
        if (!check.Process(result))
            return false;

        _queue.pop_front();
        return true;
    }

    /// Peeks at the top of the queue without removing it.
    T& peek()
    {
        std::lock_guard lock(_lock);
        return _queue.front();
    }

    /// Cancels the queue, preventing further processing of items.
    void cancel()
    {
        _canceled.store(true, std::memory_order_release);
    }

    /// Checks if the queue has been canceled.
    bool cancelled() const
    {
        return _canceled.load(std::memory_order_acquire);
    }

    /// Checks if the queue is empty.
    bool empty() const
    {
        std::lock_guard lock(_lock);
        return _queue.empty();
    }

    /// Removes the item at the front of the queue.
    void pop_front()
    {
        std::lock_guard lock(_lock);
        _queue.pop_front();
    }
};

#endif
