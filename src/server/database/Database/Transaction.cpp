#include "Transaction.h"

#include <sstream>
#include <thread>

#include "Errors.h"
#include "Log.h"
#include "PSQLConnection.h"
#include "PreparedStatement.h"
#include "Timer.h"

std::mutex TransactionTask::_deadlockLock;

constexpr Milliseconds DEADLOCK_MAX_RETRY_TIME_MS = 1min;

//- Append a raw ad-hoc query to the transaction
void TransactionBase::Append(const std::string_view sql)
{
    SQLElementData data = {};
    data.type = SQL_ELEMENT_RAW;
    data.element = std::string(sql);
    m_queries.emplace_back(data);
}

//- Append a prepared statement to the transaction
void TransactionBase::AppendPreparedStatement(PreparedStatementBase* statement)
{
    SQLElementData data = {};
    data.type = SQL_ELEMENT_PREPARED;
    data.element = statement;
    m_queries.emplace_back(data);
}

void TransactionBase::Cleanup()
{
    // This might be called by explicit calls to clean up or by the auto-destructor
    if (_cleanedUp)
        return;

    for (auto& [stmtVar, type] : m_queries)
    {
        switch (type)
        {
            case SQL_ELEMENT_PREPARED:
            {
                try
                {
                    const PreparedStatementBase* stmt = std::get<PreparedStatementBase*>(stmtVar);
                    ASSERT(stmt);
                    delete stmt;
                }
                catch (const std::bad_variant_access& ex)
                {
                    LOG_FATAL("sql.sql", "> PreparedStatementBase not found in SQLElementData. {}", ex.what());
                    ABORT();
                }
            }
            break;
            case SQL_ELEMENT_RAW:
            {
                try
                {
                    std::get<std::string>(stmtVar).clear();
                }
                catch (const std::bad_variant_access& ex)
                {
                    LOG_FATAL("sql.sql", "> std::string not found in SQLElementData. {}", ex.what());
                    ABORT();
                }
            }
            break;
        }
    }

    m_queries.clear();
    _cleanedUp = true;
}

bool TransactionTask::Execute()
{
    const PostgresResultType errorCode = TryExecute();

    if (!errorCode)
        return true;

    if (errorCode == PG_RESULT_ROLLBACK)
    {
        std::ostringstream threadIdStream;
        threadIdStream << std::this_thread::get_id();
        std::string threadId = threadIdStream.str();

        {
            // Make sure only 1 async thread retries a transaction so they don't keep deadlocking each other
            std::lock_guard lock(_deadlockLock);

            for (Milliseconds loopDuration{}, startMSTime = GetTimeMS(); loopDuration <= DEADLOCK_MAX_RETRY_TIME_MS; loopDuration = GetMSTimeDiffToNow(startMSTime))
            {
                if (!TryExecute())
                    return true;

                LOG_WARN("sql.sql", "Deadlocked SQL Transaction, retrying. Loop timer: {} ms, Thread Id: {}", loopDuration.count(), threadId);
            }
        }

        LOG_ERROR("sql.sql", "Fatal deadlocked SQL Transaction, it will not be retried anymore. Thread Id: {}", threadId);
    }

    // Clean up now.
    CleanupOnFailure();

    return false;
}

PostgresResultType TransactionTask::TryExecute() const
{
    return m_conn->ExecuteTransaction(m_trans);
}

void TransactionTask::CleanupOnFailure() const
{
    m_trans->Cleanup();
}

bool TransactionWithResultTask::Execute()
{
    const PostgresResultType errorCode = TryExecute();
    if (!errorCode)
    {
        m_result.set_value(true);
        return true;
    }

    if (errorCode == PG_RESULT_ROLLBACK)
    {
        std::ostringstream threadIdStream;
        threadIdStream << std::this_thread::get_id();
        std::string threadId = threadIdStream.str();

        {
            // Make sure only 1 async thread retries a transaction so they don't keep deadlocking each other
            std::lock_guard lock(_deadlockLock);

            for (Milliseconds loopDuration{}, startMSTime = GetTimeMS(); loopDuration <= DEADLOCK_MAX_RETRY_TIME_MS; loopDuration = GetMSTimeDiffToNow(startMSTime))
            {
                if (!TryExecute())
                {
                    m_result.set_value(true);
                    return true;
                }

                LOG_WARN("sql.sql", "Deadlocked SQL Transaction, retrying. Loop timer: {} ms, Thread Id: {}", loopDuration.count(), threadId);
            }
        }

        LOG_ERROR("sql.sql", "Fatal deadlocked SQL Transaction, it will not be retried anymore. Thread Id: {}", threadId);
    }

    // Clean up now.
    CleanupOnFailure();
    m_result.set_value(false);

    return false;
}

bool TransactionCallback::InvokeIfReady()
{
    if (m_future.valid() && m_future.wait_for(0s) == std::future_status::ready)
    {
        m_callback(m_future.get());
        return true;
    }

    return false;
}
