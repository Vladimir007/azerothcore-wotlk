#include "DatabaseWorkerPool.h"

#include <limits>
#include <vector>

#include "CharacterDatabase.h"
#include "Errors.h"
#include "Log.h"
#include "LoginDatabase.h"
#include "PostgreSQLPreparedStatement.h"
#include "PCQueue.h"
#include "PreparedStatement.h"
#include "QueryCallback.h"
#include "QueryHolder.h"
#include "QueryResult.h"
#include "SQLOperation.h"
#include "Transaction.h"
#include "WorldDatabase.h"

class PingOperation : public SQLOperation
{
public:
    //! Operation for idle delay threads
    bool Execute() override
    {
        m_conn->Ping();
        return true;
    }
};

template <class T>
DatabaseWorkerPool<T>::DatabaseWorkerPool(): _queue(new ProducerConsumerQueue<SQLOperation*>())
{
}

template <class T>
DatabaseWorkerPool<T>::~DatabaseWorkerPool()
{
    _queue->Cancel();
}

template <class T>
void DatabaseWorkerPool<T>::SetConnectionStr(const std::string_view infoString)
{
    _connectionStr = infoString;
}

template <class T>
bool DatabaseWorkerPool<T>::Open()
{
    WPFatal(!_connectionStr.empty(), "Connection info was not set!");
    LOG_INFO("sql.driver", "Opening DatabasePool.");

    if (!OpenConnection(IDX_ASYNC))
        return false;

    if (!OpenConnection(IDX_SYNCH))
    {
        LOG_INFO("sql.driver", "DatabasePool opened successfully.");
        return false;
    }

    LOG_INFO("sql.driver", " ");
    return true;
}

template <class T>
void DatabaseWorkerPool<T>::Close()
{
    LOG_INFO("sql.driver", "Closing down DatabasePool. Waiting for {} queries to finish...", _queue->Size());

    // Gracefully close async query queue, worker threads will block when the destructor
    // is called from the .clear() functions below until the queue is empty
    _queue->Shutdown();

    //! Closes the actualy PostgreSQL connection.
    _connections[IDX_ASYNC].reset();

    LOG_INFO("sql.driver", "Asynchronous connections on DatabasePool terminated. Proceeding with synchronous connections.");

    //! Shut down the synchronous connection
    //! There's no need for locking the connection, because DatabaseWorkerPool<>::Close
    //! should only be called after any other thread tasks in the core have exited,
    //! meaning there can be no concurrent access at this point.
    _connections[IDX_SYNCH].reset();

    LOG_INFO("sql.driver", "All connections on DatabasePool closed.");
}

template <class T>
bool DatabaseWorkerPool<T>::PrepareStatements()
{
    for (auto const& connection : _connections)
    {
        connection->LockIfReady();
        if (!connection->PrepareStatements())
        {
            connection->Unlock();
            Close();
            return false;
        }
        connection->Unlock();

        std::size_t const preparedSize = connection->m_stmts.size();
        if (_preparedStatementSize.size() < preparedSize)
            _preparedStatementSize.resize(preparedSize);

        for (std::size_t i = 0; i < preparedSize; ++i)
        {
            // Already set by another connection
            // (each connection only has prepared statements of its own type sync/async)
            if (_preparedStatementSize[i] > 0)
                continue;

            if (const PSQLPreparedStatement* stmt = connection->m_stmts[i].get())
            {
                uint32 const paramCount = stmt->GetParameterCount();
                ASSERT(paramCount < std::numeric_limits<uint8>::max());
                _preparedStatementSize[i] = static_cast<uint8>(paramCount);
            }
        }
    }
    return true;
}


template <class T>
template <typename ... Args>
QueryResult DatabaseWorkerPool<T>::Query(std::string_view sql, Args&&... args)
{
    if (sql.empty())
        return QueryResult(nullptr);
    auto connection = GetFreeConnection();
    ResultSet* result = connection->Query(sql, std::forward<Args>(args)...);
    connection->Unlock();

    if (!result || !result->GetRowCount())
    {
        delete result;
        return QueryResult(nullptr);
    }
    return QueryResult(result);
}

template <class T>
QueryResult DatabaseWorkerPool<T>::Query(PreparedStatement<T>* stmt)
{
    auto connection = GetFreeConnection();
    ResultSet* result = connection->Query(stmt);
    connection->Unlock();

    //! Delete proxy-class. Not needed anymore
    delete stmt;

    if (!result || !result->GetRowCount())
    {
        delete result;
        return QueryResult(nullptr);
    }

    return QueryResult(result);
}

template <class T>
QueryCallback DatabaseWorkerPool<T>::AsyncQuery(PreparedStatement<T>* stmt)
{
    const auto task = new PreparedStatementTask(stmt, true);
    // Store future result before enqueueing - task might get already processed and deleted before returning from this method
    QueryResultFuture result = task->GetFuture();
    Enqueue(task);
    return QueryCallback(std::move(result), true);
}

template <class T>
SQLQueryHolderCallback DatabaseWorkerPool<T>::DelayQueryHolder(std::shared_ptr<SQLQueryHolder<T>> holder)
{
    const auto task = new SQLQueryHolderTask(holder);
    // Store future result before enqueueing - task might get already processed and deleted before returning from this method
    QueryResultHolderFuture result = task->GetFuture();
    Enqueue(task);
    return { std::move(holder), std::move(result) };
}

template <class T>
SQLTransaction<T> DatabaseWorkerPool<T>::BeginTransaction()
{
    return std::make_shared<Transaction<T>>();
}

template <class T>
void DatabaseWorkerPool<T>::CommitTransaction(SQLTransaction<T> transaction)
{
    Enqueue(new TransactionTask(transaction));
}

template <class T>
TransactionCallback DatabaseWorkerPool<T>::AsyncCommitTransaction(SQLTransaction<T> transaction)
{
    const auto task = new TransactionWithResultTask(transaction);
    TransactionFuture result = task->GetFuture();
    Enqueue(task);
    return TransactionCallback(std::move(result));
}

template <class T>
void DatabaseWorkerPool<T>::DirectCommitTransaction(SQLTransaction<T>& transaction)
{
    T* connection = GetFreeConnection();
    const PostgresResultType errorCode = connection->ExecuteTransaction(transaction);

    if (!errorCode)
    {
        connection->Unlock(); // OK, operation successful
        return;
    }

    if (errorCode == PG_RESULT_ROLLBACK)
    {
        for (uint8 i = 0; i < 5; ++i)
        {
            if (!connection->ExecuteTransaction(transaction))
                break;
        }
    }

    //! Clean up now.
    transaction->Cleanup();
    connection->Unlock();
}

template <class T>
PreparedStatement<T>* DatabaseWorkerPool<T>::GetPreparedStatement(PreparedStatementIndex index)
{
    return new PreparedStatement<T>(index, _preparedStatementSize[index]);
}

template <class T>
void DatabaseWorkerPool<T>::KeepAlive()
{
    //! Ping synchronous connections
    T* connection = _connections[IDX_SYNCH].get();
    if (connection && connection->LockIfReady())
    {
        connection->Ping();
        connection->Unlock();
    }

    //! Assuming all worker threads are free, every worker thread will receive 1 ping operation request
    //! If one or more worker threads are busy, the ping operations will not be split evenly, but this doesn't matter
    //! as the sole purpose is to prevent connections from idling.
    if (_connections[IDX_ASYNC])
        Enqueue(new PingOperation);
}

template <class T>
bool DatabaseWorkerPool<T>::OpenConnection(InternalIndex type)
{
    // Create the connection
    auto connection = [&]
    {
        switch (type)
        {
        case IDX_ASYNC:
            return std::make_unique<T>(_queue.get(), _connectionStr);
        case IDX_SYNCH:
            return std::make_unique<T>(_connectionStr);
        default:
            ABORT();
        }
    }();

    if (!connection->Open())
    {
        // Failed to open a connection, abort and cleanup
        _queue->Cancel();
        _connections[type].reset();
        return false;
    }
    _connections[type] = std::move(connection);
    return true;
}

template <class T>
void DatabaseWorkerPool<T>::Enqueue(SQLOperation* op) const
{
    _queue->Push(op);
}

template <class T>
std::size_t DatabaseWorkerPool<T>::QueueSize() const
{
    return _queue->Size();
}

template <class T>
T* DatabaseWorkerPool<T>::GetFreeConnection()
{
    T* connection = _connections[IDX_SYNCH].get();

    //! Block forever until a connection is free
    while (true)
    {
        //! Must be matched with t->Unlock() or you will get deadlocks
        if (connection->LockIfReady())
            break;
    }

    return connection;
}

template <class T>
void DatabaseWorkerPool<T>::Execute(PreparedStatement<T>* stmt)
{
    const auto task = new PreparedStatementTask(stmt);
    Enqueue(task);
}

template <class T>
void DatabaseWorkerPool<T>::DirectExecute(PreparedStatement<T>* stmt)
{
    T* connection = GetFreeConnection();
    connection->Execute(stmt);
    connection->Unlock();

    //! Delete proxy-class. Not needed anymore
    delete stmt;
}

template <class T>
void DatabaseWorkerPool<T>::ExecuteOrAppend(SQLTransaction<T>& trans, PreparedStatement<T>* stmt)
{
    if (!trans)
        Execute(stmt);
    else
        trans->Append(stmt);
}

template class DatabaseWorkerPool<LoginDatabaseConnection>;
template class DatabaseWorkerPool<WorldDatabaseConnection>;
template class DatabaseWorkerPool<CharacterDatabaseConnection>;
