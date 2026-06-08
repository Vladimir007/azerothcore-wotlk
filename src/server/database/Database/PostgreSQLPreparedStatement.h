#ifndef PQ_SQL_PREPARED_STATEMENT_H
#define PQ_SQL_PREPARED_STATEMENT_H

#include <string>
#include <vector>
#include <pqxx/pqxx>

#include "DatabaseEnvFwd.h"
#include "Define.h"


class PSQLPreparedStatement
{
friend class PSQLConnection;
friend class PreparedStatementBase;

public:
    PSQLPreparedStatement(std::string_view name, std::string_view queryString);
    ~PSQLPreparedStatement() = default;

    pqxx::params BindParameters(const PreparedStatementBase* stmt);

    uint32 GetParameterCount() const { return m_paramCount; }

protected:
    std::string GetName() { return m_name; }
    std::string getQueryString() const { return m_queryString; }

private:
    template<typename T>
    static void AddParameter(T value, pqxx::params& params);

    static void AddParameter(int8 value, pqxx::params& params);
    static void AddParameter(uint8 value, pqxx::params& params);
    static void AddParameter(std::vector<uint8> const& value, pqxx::params& params);
    static uint32 ExtractParamCount(std::string_view sql);

    std::string m_name;

    uint32 m_paramCount;
    std::string m_queryString{};

    PSQLPreparedStatement(PSQLPreparedStatement const& right) = delete;
    PSQLPreparedStatement& operator=(PSQLPreparedStatement const& right) = delete;
};

#endif
