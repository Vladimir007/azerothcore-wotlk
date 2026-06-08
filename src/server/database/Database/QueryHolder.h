#ifndef QUERY_HOLDER_H
#define QUERY_HOLDER_H

#include <vector>
#include "SQLOperation.h"

class SQLQueryHolderBase
{
friend class SQLQueryHolderTask;

public:
    SQLQueryHolderBase() = default;
    virtual ~SQLQueryHolderBase();
    void SetSize(std::size_t size);
    QueryResult GetPreparedResult(std::size_t index) const;
    void SetPreparedResult(std::size_t index, ResultSet* result);

protected:
    bool SetPreparedQueryImpl(std::size_t index, PreparedStatementBase* stmt);

private:
    std::vector<std::pair<PreparedStatementBase*, QueryResult>> m_queries;
};

template<typename T>
class SQLQueryHolder : public SQLQueryHolderBase
{
public:
    bool SetPreparedQuery(std::size_t index, PreparedStatement<T>* stmt)
    {
        return SetPreparedQueryImpl(index, stmt);
    }
};

class AC_DATABASE_API SQLQueryHolderTask : public SQLOperation
{
public:
    explicit SQLQueryHolderTask(std::shared_ptr<SQLQueryHolderBase> holder)
        : m_holder(std::move(holder)) { }

    ~SQLQueryHolderTask() override;

    bool Execute() override;
    QueryResultHolderFuture GetFuture() { return m_result.get_future(); }

private:
    std::shared_ptr<SQLQueryHolderBase> m_holder;
    QueryResultHolderPromise m_result;
};

class AC_DATABASE_API SQLQueryHolderCallback
{
public:
    SQLQueryHolderCallback(std::shared_ptr<SQLQueryHolderBase>&& holder, QueryResultHolderFuture&& future)
        : m_holder(std::move(holder)), m_future(std::move(future)) { }

    SQLQueryHolderCallback(SQLQueryHolderCallback&&) = default;
    SQLQueryHolderCallback& operator=(SQLQueryHolderCallback&&) = default;

    void AfterComplete(std::function<void(SQLQueryHolderBase const&)> callback) &
    {
        m_callback = std::move(callback);
    }

    bool InvokeIfReady() const;

    std::shared_ptr<SQLQueryHolderBase> m_holder;
    QueryResultHolderFuture m_future;
    std::function<void(SQLQueryHolderBase const&)> m_callback;
};

#endif
