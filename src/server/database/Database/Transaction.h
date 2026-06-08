#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <functional>
#include <mutex>
#include <vector>

#include "DatabaseEnvFwd.h"
#include "SQLOperation.h"
#include "StringFormat.h"

/*! Transactions, high level class. */
class TransactionBase
{
    friend class TransactionTask;
    friend class PSQLConnection;

    template <typename T>
    friend class DatabaseWorkerPool;

public:
    TransactionBase()  = default;
    virtual ~TransactionBase() { Cleanup(); }

    void Append(std::string_view sql);

    template<typename... Args>
    void Append(std::string_view sql, Args&&... args)
    {
        Append(Acore::StringFormat(sql, std::forward<Args>(args)...));
    }

    [[nodiscard]] std::size_t GetSize() const { return m_queries.size(); }

protected:
    void AppendPreparedStatement(PreparedStatementBase* statement);
    void Cleanup();
    std::vector<SQLElementData> m_queries;

private:
    bool _cleanedUp{false};
};

template<typename T>
class Transaction : public TransactionBase
{
public:
    using TransactionBase::Append;

    void Append(PreparedStatement<T>* statement)
    {
        AppendPreparedStatement(statement);
    }
};

/*! Low level class*/
class TransactionTask : public SQLOperation
{
    template <class T>
    friend class DatabaseWorkerPool;

    friend class DatabaseWorker;
    friend class TransactionCallback;

public:
    explicit TransactionTask(std::shared_ptr<TransactionBase> trans) : m_trans(std::move(trans)) { }
    ~TransactionTask() override = default;
    bool Execute() override;

protected:
    PostgresResultType TryExecute() const;
    void CleanupOnFailure() const;

    std::shared_ptr<TransactionBase> m_trans;
    static std::mutex _deadlockLock;
};

class TransactionWithResultTask : public TransactionTask
{
public:
    explicit TransactionWithResultTask(const std::shared_ptr<TransactionBase>& trans) : TransactionTask(trans) { }

    TransactionFuture GetFuture() { return m_result.get_future(); }
    bool Execute() override;

protected:
    TransactionPromise m_result;
};

class TransactionCallback
{
public:
    explicit TransactionCallback(TransactionFuture&& future) : m_future(std::move(future)) { }
    TransactionCallback(TransactionCallback&&) = default;

    TransactionCallback& operator=(TransactionCallback&&) = default;

    void AfterComplete(std::function<void(bool)> callback) &
    {
        m_callback = std::move(callback);
    }

    bool InvokeIfReady();

    TransactionFuture m_future;
    std::function<void(bool)> m_callback;
};

#endif
