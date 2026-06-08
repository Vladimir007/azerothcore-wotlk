#include "QueryHolder.h"

#include "Errors.h"
#include "Log.h"
#include "PSQLConnection.h"
#include "PreparedStatement.h"
#include "QueryResult.h"

bool SQLQueryHolderBase::SetPreparedQueryImpl(const std::size_t index, PreparedStatementBase* stmt)
{
    if (m_queries.size() <= index)
    {
        LOG_ERROR("sql.sql", "Query index ({}) out of range (size: {}) for prepared statement",
            static_cast<uint32>(index), static_cast<uint32>(m_queries.size()));
        return false;
    }

    m_queries[index].first = stmt;
    return true;
}

QueryResult SQLQueryHolderBase::GetPreparedResult(std::size_t index) const
{
    // Don't call to this function if the index is of a prepared statement
    ASSERT(index < m_queries.size(), "Query holder result index out of range, tried to access index {} but there are only {} results",
        index, m_queries.size());

    return m_queries[index].second;
}

void SQLQueryHolderBase::SetPreparedResult(const std::size_t index, ResultSet* result)
{
    if (result && !result->GetRowCount())
    {
        delete result;
        result = nullptr;
    }

    /// Store the result in the holder
    if (index < m_queries.size())
        m_queries[index].second = QueryResult(result);
}

SQLQueryHolderBase::~SQLQueryHolderBase()
{
    for (const auto& key : m_queries | std::views::keys)
    {
        /// If the result was never used, free the resources
        /// results used already (getresult called) are expected to be deleted
        delete key;
    }
}

void SQLQueryHolderBase::SetSize(const std::size_t size)
{
    /// To optimize push_back, reserve the number of queries about to be executed
    m_queries.resize(size);
}

SQLQueryHolderTask::~SQLQueryHolderTask() = default;

bool SQLQueryHolderTask::Execute()
{
    /// execute all queries in the holder and pass the results
    for (std::size_t i = 0; i < m_holder->m_queries.size(); ++i)
        if (const PreparedStatementBase* stmt = m_holder->m_queries[i].first)
            m_holder->SetPreparedResult(i, m_conn->Query(stmt));

    m_result.set_value();
    return true;
}

bool SQLQueryHolderCallback::InvokeIfReady() const
{
    if (m_future.valid() && m_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        m_callback(*m_holder);
        return true;
    }
    return false;
}
