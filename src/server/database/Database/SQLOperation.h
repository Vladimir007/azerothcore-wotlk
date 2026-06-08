#ifndef SQL_OPERATION_H
#define SQL_OPERATION_H

#include <variant>
#include "DatabaseEnvFwd.h"

//- Type specifier of our element data
enum SQLElementDataType
{
    SQL_ELEMENT_RAW,
    SQL_ELEMENT_PREPARED
};

//- The element
struct SQLElementData
{
    std::variant<PreparedStatementBase*, std::string> element;
    SQLElementDataType type;
};

class PSQLConnection;

class SQLOperation
{
public:
    SQLOperation() = default;
    virtual ~SQLOperation() = default;

    virtual int call()
    {
        Execute();
        return 0;
    }

    virtual bool Execute() = 0;
    virtual void SetConnection(PSQLConnection* con) { m_conn = con; }

    PSQLConnection* m_conn{nullptr};

private:
    SQLOperation(SQLOperation const& right) = delete;
    SQLOperation& operator=(SQLOperation const& right) = delete;
};

#endif
