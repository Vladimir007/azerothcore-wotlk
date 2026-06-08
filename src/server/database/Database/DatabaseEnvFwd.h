#ifndef DATABASE_ENV_FWD_H
#define DATABASE_ENV_FWD_H

#include <future>

enum PostgresResultType
{
    PG_RESULT_SUCCESS,
    PG_RESULT_SQL_ERROR,
    PG_RESULT_USAGE_ERROR,
    PG_RESULT_DISCONNECTED,
    PG_RESULT_ROLLBACK,
    PG_RESULT_UNKNOWN
};

class Field;

class ResultSet;
using QueryResult = std::shared_ptr<ResultSet>;
using QueryResultFuture = std::future<QueryResult>;
using QueryResultPromise = std::promise<QueryResult>;

class CharacterDatabaseConnection;
class LoginDatabaseConnection;
class WorldDatabaseConnection;

class PreparedStatementBase;

template<typename T>
class PreparedStatement;

using CharacterDatabasePreparedStatement = PreparedStatement<CharacterDatabaseConnection>;
using LoginDatabasePreparedStatement = PreparedStatement<LoginDatabaseConnection>;
using WorldDatabasePreparedStatement = PreparedStatement<WorldDatabaseConnection>;

class QueryCallback;

template<typename T>
class AsyncCallbackProcessor;

using QueryCallbackProcessor = AsyncCallbackProcessor<QueryCallback>;

class TransactionBase;

using TransactionFuture = std::future<bool>;
using TransactionPromise = std::promise<bool>;

template<typename T>
class Transaction;

class TransactionCallback;

template<typename T>
using SQLTransaction = std::shared_ptr<Transaction<T>>;

using CharacterDatabaseTransaction = SQLTransaction<CharacterDatabaseConnection>;
using LoginDatabaseTransaction = SQLTransaction<LoginDatabaseConnection>;
using WorldDatabaseTransaction = SQLTransaction<WorldDatabaseConnection>;

class SQLQueryHolderBase;
using QueryResultHolderFuture = std::future<void>;
using QueryResultHolderPromise = std::promise<void>;

template<typename T>
class SQLQueryHolder;

using CharacterDatabaseQueryHolder = SQLQueryHolder<CharacterDatabaseConnection>;
using LoginDatabaseQueryHolder = SQLQueryHolder<LoginDatabaseConnection>;
using WorldDatabaseQueryHolder = SQLQueryHolder<WorldDatabaseConnection>;

class SQLQueryHolderCallback;

#endif
