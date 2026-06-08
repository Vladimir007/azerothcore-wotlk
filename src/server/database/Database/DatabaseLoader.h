#ifndef DATABASE_LOADER_H
#define DATABASE_LOADER_H

#include <functional>
#include <queue>
#include <stack>
#include <string>

template <class T>
class DatabaseWorkerPool;

// A helper class to initiate all database worker pools,
// handles updating, delays preparing of statements and cleans up on failure.
class DatabaseLoader
{
public:
    explicit DatabaseLoader(std::string const& logger);

    template <class T>
    DatabaseLoader& AddDatabase(DatabaseWorkerPool<T>& pool, std::string const& name);

    // Load all databases
    bool Load();

private:
    using Predicate = std::function<bool()>;
    using Closer = std::function<void()>;

    bool Process(std::queue<Predicate>& queue);

    std::string const _logger;

    std::queue<Predicate> _open, _prepare;
    std::stack<Closer> _close;
};

#endif
