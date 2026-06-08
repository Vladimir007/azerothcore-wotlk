#include "PSQLConnection.h"

#include <pqxx/pqxx>
#include <utility>

#include "DatabaseWorker.h"
#include "Log.h"
#include "PostgreSQLPreparedStatement.h"
#include "PreparedStatement.h"
#include "QueryResult.h"
#include "StringConvert.h"
#include "Timer.h"
#include "Transaction.h"

#define PREPARED_STMT_NAME "prepped_stmt_{}"

PSQLConnection::PSQLConnection(std::string connectionStr) :
    m_reconnecting(false),
    m_prepareError(false),
    _queue(nullptr),
    _connectionFlags(CONNECTION_SYNCH),
    _connectionStr(std::move(connectionStr)),
    _connection(nullptr),
    _transaction(nullptr) { }

PSQLConnection::PSQLConnection(ProducerConsumerQueue<SQLOperation*>* queue, std::string  connectionStr) :
    m_reconnecting(false),
    m_prepareError(false),
    _queue(queue),
    _connectionFlags(CONNECTION_ASYNC),
    _connectionStr(std::move(connectionStr)),
    _connection(nullptr),
    _transaction(nullptr)
{
    _worker = std::make_unique<DatabaseWorker>(_queue, this);
}

PSQLConnection::~PSQLConnection()
{
    Close();
}

bool PSQLConnection::Open()
{
    if (connect())
    {
        LOG_INFO("sql.sql", "Connected to PostgresSQL database '{}' at {}", _connection->dbname(), _connection->hostname());
        return true;
    }
    return false;
}

void PSQLConnection::Close()
{
    // Stop the worker thread before the statements are cleared
    _worker.reset();
    m_stmts.clear();

    RollbackTransaction();
    if (_connection)
        _connection.reset();
}

bool PSQLConnection::PrepareStatements()
{
    DoPrepareStatements();
    return !m_prepareError;
}

template <typename... Args>
bool PSQLConnection::Execute(std::string_view query, Args&&... args) {
    if (!_connection)
        return false;

    pqxx::params params;

    ([&](auto&& arg) {
        params.append(arg);
    }(std::forward<Args>(args)), ...);

    const uint32 _s = getMSTime();
    const bool result = safeExecute([&] {
        if (_transaction)
            _transaction->exec(query, params);
        else
        {
            pqxx::work txn{*_connection};
            txn.exec(query, params);
            txn.commit();
        }
    }, query);

    LOG_DEBUG("sql.sql", "[{} ms] SQL: {}", getMSTimeDiff(_s, getMSTime()), query);
    return result;
}

bool PSQLConnection::Execute(const PreparedStatementBase* stmt)
{
    if (!_connection)
        return false;

    PSQLPreparedStatement* m_mStmt = GetPreparedStatement(stmt->GetIndex());
    ASSERT(m_mStmt);

    const pqxx::params params = m_mStmt->BindParameters(stmt);

    const uint32 _s = getMSTime();

    const bool result = safeExecute([&] {
        if (_transaction)
            _transaction->exec(pqxx::prepped{m_mStmt->GetName()}, params);
        else
        {
            pqxx::work txn{*_connection};
            txn.exec(pqxx::prepped{m_mStmt->GetName()}, params);
            txn.commit();
        }
    }, m_mStmt->getQueryString());

    LOG_DEBUG("sql.sql", "[{} ms] SQL(p): {}", getMSTimeDiff(_s, getMSTime()), m_mStmt->getQueryString());
    return result;
}

template <typename... Args>
ResultSet* PSQLConnection::Query(std::string_view query, Args&&... args) {
    if (!_connection || _transaction)
        return nullptr;

    pqxx::params params;

    ([&](auto&& arg) {
        params.append(arg);
    }(std::forward<Args>(args)), ...);

    pqxx::result queryRes;

    const uint32 _s = getMSTime();
    const bool result = safeExecute([&] {
        pqxx::work txn{*_connection};
        queryRes = txn.exec(query, params);
        txn.commit();
    }, query);

    LOG_DEBUG("sql.sql", "[{} ms] SQL: {}", getMSTimeDiff(_s, getMSTime()), query);
    if (!result)
        return nullptr;
    return new ResultSet(queryRes);
}

ResultSet* PSQLConnection::Query(const PreparedStatementBase* stmt)
{
    if (!_connection || _transaction)
        return nullptr;

    PSQLPreparedStatement* m_mStmt = GetPreparedStatement(stmt->GetIndex());
    ASSERT(m_mStmt);

    const pqxx::params params = m_mStmt->BindParameters(stmt);

    const uint32 _s = getMSTime();
    pqxx::result queryRes;

    const bool result = safeExecute([&] {
        pqxx::work txn{*_connection};
        queryRes = txn.exec(pqxx::prepped{m_mStmt->GetName()}, params);
        txn.commit();
    }, m_mStmt->getQueryString());

    LOG_DEBUG("sql.sql", "[{} ms] SQL(p): {}", getMSTimeDiff(_s, getMSTime()), m_mStmt->getQueryString());
    if (!result)
        return nullptr;
    return new ResultSet(queryRes);
}

bool PSQLConnection::BeginTransaction()
{
    if (!_connection || _transaction)
        return false;
    return safeExecute([&] {
        _transaction = std::make_unique<pqxx::work>(*_connection);
    }, "StartTransaction");
}

void PSQLConnection::RollbackTransaction()
{
    if (!_transaction)
        return;
    try
    {
        _transaction->abort();
    } catch (std::exception&) {}

    _transaction.reset();
}

bool PSQLConnection::CommitTransaction()
{
    if (!_transaction)
        return false;
    const bool result = safeExecute([&] { _transaction->commit(); }, "CommitTransaction");
    _transaction.reset();
    return result;
}

PostgresResultType PSQLConnection::ExecuteTransaction(const std::shared_ptr<TransactionBase>& transaction)
{
    std::vector<SQLElementData> const& queries = transaction->m_queries;
    if (queries.empty())
        return PG_RESULT_SUCCESS;

    BeginTransaction();

    for (const auto& [element, type] : queries)
    {
        switch (type)
        {
        case SQL_ELEMENT_PREPARED:
            {
                const PreparedStatementBase* stmt = nullptr;

                try
                {
                    stmt = std::get<PreparedStatementBase*>(element);
                }
                catch (const std::bad_variant_access& ex)
                {
                    LOG_FATAL("sql.sql", "> PreparedStatementBase not found in SQLElementData. {}", ex.what());
                    ABORT();
                }

                ASSERT(stmt);

                if (!Execute(stmt))
                {
                    LOG_WARN("sql.sql", "Transaction aborted. {} queries not executed.", queries.size());
                    RollbackTransaction();
                    return GetLastError();
                }
            }
            break;

        case SQL_ELEMENT_RAW:
            {
                std::string sql{};

                try
                {
                    sql = std::get<std::string>(element);
                }
                catch (const std::bad_variant_access& ex)
                {
                    LOG_FATAL("sql.sql", "> std::string not found in SQLElementData. {}", ex.what());
                    ABORT();
                }

                ASSERT(!sql.empty());

                if (!Execute(sql))
                {
                    LOG_WARN("sql.sql", "Transaction aborted. {} queries not executed.", queries.size());
                    RollbackTransaction();
                    return GetLastError();
                }
            }
            break;
        }
    }
    CommitTransaction();
    return PG_RESULT_SUCCESS;
}

void PSQLConnection::Ping()
{
    try {
        pqxx::work tx(*_connection);
        tx.exec("SELECT 1");
        tx.commit();
    } catch (const pqxx::broken_connection&) {
        connect();
    }
}

bool PSQLConnection::LockIfReady()
{
    return _mutex.try_lock();
}

void PSQLConnection::Unlock()
{
    _mutex.unlock();
}
PSQLPreparedStatement* PSQLConnection::GetPreparedStatement(uint32 index) const
{
    ASSERT(index < m_stmts.size(), "Tried to access invalid prepared statement index {} (max index {}), connection type: {}",
        index, m_stmts.size(), (_connectionFlags & CONNECTION_ASYNC) ? "asynchronous" : "synchronous");

    PSQLPreparedStatement* ret = m_stmts[index].get();

    if (!ret)
        LOG_ERROR("sql.sql", "Could not fetch prepared statement {}, connection type: {}.",
            index, (_connectionFlags & CONNECTION_ASYNC) ? "asynchronous" : "synchronous");

    return ret;
}

void PSQLConnection::PrepareStatement(uint32 index, std::string_view sql, const ConnectionFlags flags)
{
    // Check if specified query should be prepared on this connection
    // i.e. don't prepare async statements on synchronous connections
    // to save memory that will not be used.
    if (!(_connectionFlags & flags))
    {
        m_stmts[index].reset();
        return;
    }
    std::string name = Acore::StringFormat(PREPARED_STMT_NAME, index);
    try
    {
        _connection->prepare(name, sql);
    }
    catch (std::exception& ex)
    {
        LOG_ERROR("sql.sql", "In pqxx::connection::prepare() id: {}, sql: \"{}\"", index, sql);
        LOG_ERROR("sql.sql", "{}", ex.what());
        m_prepareError = true;
    }
    m_stmts[index] = std::make_unique<PSQLPreparedStatement>(name, sql);
}

bool PSQLConnection::connect() {
    try {
        _connection = std::make_unique<pqxx::connection>(_connectionStr);
        return _connection && _connection->is_open();
    } catch (const std::exception& e) {
        LOG_ERROR("sql.driver", "Could not connect to PostgresSQL database {}: {}", _connectionStr, e.what());
        return false;
    }
}

template <typename Func>
bool PSQLConnection::safeExecute(Func&& func, const std::string_view context) {
    try {
        func();
        _lastError = PG_RESULT_SUCCESS;
        return true;
    } catch (const std::exception& e) {
        if (handlePSQLException(e, context))
            return safeExecute(func, context);  // Try again
        return false;
    }
}

bool PSQLConnection::handlePSQLException(const std::exception&, const std::string_view context) {
    try {
        throw;
    }
    catch (const pqxx::broken_connection& connErr)
    {
        LOG_INFO("sql.sql", "[{}] Attempting to reconnect to the Postgresql server...", context);
        for (int i = 0; i < MAX_ATTEMPTS; i++)
        {
            if (connect())
            {
                LOG_INFO("sql.sql", "[{}] Successfully reconnected to {} @{}:{} ({}).",
                    context, _connection->dbname(), _connection->hostname(),
                    _connection->port_number() ? _connection->port_number().value() : 5432,
                    (_connectionFlags & CONNECTION_ASYNC) ? "asynchronous" : "synchronous");
                return !_transaction;
            }
            std::this_thread::sleep_for(3s); // Sleep 3 seconds
        }

        // Shut down the server when the Postgresql server isn't reachable for some time
        std::string str = "Failed to reconnect to the Postgresql server, terminating the server to prevent data corruption!";
        LOG_FATAL("sql.sql", "[{}] {}", context, str);

        _connection.reset();
        _lastError = PG_RESULT_DISCONNECTED;

        // We could also initiate a shutdown through using std::raise(SIGTERM)
        std::this_thread::sleep_for(10s);
        ABORT("{}\n\n[{}] {}", str, connErr.sqlstate(), connErr.what());
    }
    catch (const pqxx::transaction_rollback&)
    {
        _lastError = PG_RESULT_ROLLBACK;
    }
    catch (const pqxx::usage_error& usageErr) {
        std::string str = "pqxx usage error. Core fix required.";
        LOG_FATAL("sql.sql", "[{}] {}", context, str);
        _lastError = PG_RESULT_USAGE_ERROR;

        std::this_thread::sleep_for(10s);
        ABORT("{}\n\n[{}] {}", str, usageErr.sqlstate(), usageErr.what());
    }
    catch (const pqxx::sql_error& sqlErr) {
        std::string str = "SQL error. Core fix required.";
        LOG_FATAL("sql.sql", "[{}] {}", context, str);
        _lastError = PG_RESULT_SQL_ERROR;

        std::this_thread::sleep_for(10s);
        ABORT("{}\n\n[{}] {}", str, sqlErr.sqlstate(), sqlErr.what());
    }
    catch (const std::exception& genericErr) {
        LOG_ERROR("sql.sql", "[{}]Unhandled PostgresSQL error. Unexpected behaviour possible.\n{}",
            context, genericErr.what());
        _lastError = PG_RESULT_UNKNOWN;
    }
    return false;
}
