#ifndef PCQ_H
#define PCQ_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class ProducerConsumerQueue
{
    mutable std::mutex _queueLock;
    std::queue<T> _queue;
    std::condition_variable _condition;
    std::atomic<bool> _cancel{};
    std::atomic<bool> _shutdown{};

public:
    ProducerConsumerQueue() = default;

    void Push(const T& value)
    {
        {
            std::lock_guard lock(_queueLock);
            _queue.push(std::move(value));
        }
        _condition.notify_one();
    }

    bool Empty() const
    {
        std::lock_guard lock(_queueLock);
        return _queue.empty();
    }

    [[nodiscard]] std::size_t Size() const
    {
        std::lock_guard lock(_queueLock);
        return _queue.size();
    }

    bool Pop(T& value)
    {
        std::lock_guard lock(_queueLock);
        if (_queue.empty() || _cancel)
            return false;

        value = std::move(_queue.front());
        _queue.pop();
        return true;
    }

    void WaitAndPop(T& value)
    {
        std::unique_lock lock(_queueLock);

        // Wait for the queue to have an element or the cancel/shutdown flag
        _condition.wait(lock, [this] { return !_queue.empty() || _cancel || _shutdown; });

        if (_queue.empty() || _cancel)
            return;

        value = std::move(_queue.front());
        _queue.pop();
    }

    // Clears the queue and immediately stops any consumers.
    void Cancel()
    {
        std::lock_guard lock(_queueLock);
        while (!_queue.empty()) {
            T& value = _queue.front();
            DeleteQueuedObject(value);
            _queue.pop();
        }
        _cancel = true;
        _condition.notify_all();
    }

    // Graceful stop: waits for the queue to become empty before stopping consumers.
    void Shutdown()
    {
        _shutdown = true;
        _condition.notify_all();
    }

private:
    template<typename E = T>
    std::enable_if_t<std::is_pointer_v<E>> DeleteQueuedObject(E& obj)
    {
        delete obj;
    }

    template<typename E = T>
    std::enable_if_t<!std::is_pointer_v<E>> DeleteQueuedObject(E const& /*obj*/) { }
};

#endif
