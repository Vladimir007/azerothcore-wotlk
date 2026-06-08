#ifndef POSTGRESQL_CONNECTION_H
#define POSTGRESQL_CONNECTION_H

#include <mutex>
#include <string>
#include <vector>
#include <pqxx/pqxx>
#include "DatabaseEnvFwd.h"
#include "Define.h"

template <typename T>
class ProducerConsumerQueue;

class DatabaseWorker;
class PSQLPreparedStatement;
class SQLOperation;

enum ConnectionFlags
{
    CONNECTION_ASYNC = 0x1,
    CONNECTION_SYNCH = 0x2,
    CONNECTION_BOTH = CONNECTION_ASYNC | CONNECTION_SYNCH
};

class PSQLConnection
{
    template <class T>
    friend class DatabaseWorkerPool;

    friend class PingOperation;

    static constexpr int MAX_ATTEMPTS = 5;

public:
    explicit PSQLConnection(std::string  connectionStr);                                      //! Constructor for synchronous connections.
    PSQLConnection(ProducerConsumerQueue<SQLOperation*>* queue, std::string  connectionStr);  //! Constructor for asynchronous connections.
    virtual ~PSQLConnection();

    PSQLConnection(PSQLConnection const& right) = delete;
    PSQLConnection& operator=(PSQLConnection const& right) = delete;

    bool Open();
    void Close();

    bool PrepareStatements();

    template <typename... Args>
    bool Execute(std::string_view query, Args&&... args);

    bool Execute(const PreparedStatementBase* stmt);

    template <typename... Args>
    ResultSet* Query(std::string_view query, Args&&... args);

    ResultSet* Query(const PreparedStatementBase* stmt);

    bool BeginTransaction();
    void RollbackTransaction();
    bool CommitTransaction();
    PostgresResultType ExecuteTransaction(const std::shared_ptr<TransactionBase>& transaction);
    void Ping();

    [[nodiscard]] PostgresResultType GetLastError() const { return _lastError; }

protected:
    /// Tries to acquire lock. If lock is acquired by another thread
    /// the calling parent will just try another connection
    bool LockIfReady();

    /// Called by parent database pool. Will let other threads access this connection
    void Unlock();

    [[nodiscard]] PSQLPreparedStatement* GetPreparedStatement(uint32 index) const;
    void PrepareStatement(uint32 index, std::string_view sql, ConnectionFlags flags);

    virtual void DoPrepareStatements() = 0;

    typedef std::vector<std::unique_ptr<PSQLPreparedStatement>> PreparedStatementContainer;

    PreparedStatementContainer m_stmts; //! PreparedStatements storage
    bool m_reconnecting;  //! Are we reconnecting?
    bool m_prepareError;  //! Was there any error while preparing statements?

private:
    bool connect();

    template <typename Func>
    bool safeExecute(Func&& func, std::string_view context);

    bool handlePSQLException(const std::exception&, std::string_view context);

    ProducerConsumerQueue<SQLOperation*>* _queue;
    std::unique_ptr<DatabaseWorker> _worker;
    ConnectionFlags _connectionFlags;
    std::mutex _mutex;
    std::string _connectionStr;
    std::unique_ptr<pqxx::connection> _connection;
    std::unique_ptr<pqxx::work> _transaction;
    PostgresResultType _lastError = PG_RESULT_SUCCESS;
};

#endif
