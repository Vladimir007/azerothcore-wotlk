#include "PreparedStatement.h"

#include "Errors.h"
#include "PSQLConnection.h"
#include "QueryResult.h"

PreparedStatementBase::PreparedStatementBase(const uint32 index, const uint8 capacity):
    m_index(index), statement_data(capacity) {}

PreparedStatementBase::~PreparedStatementBase() = default;

template<typename T>
Acore::Types::is_non_string_view_v<T> PreparedStatementBase::SetValidData(const uint8 index, T const& value)
{
    ASSERT(index < statement_data.size());
    statement_data[index].data.emplace<T>(value);
}

// Non template functions
void PreparedStatementBase::SetValidData(const uint8 index) // NOLINT(*-convert-member-functions-to-static)
{
    ASSERT(index < statement_data.size());
    statement_data[index].data.emplace<std::nullptr_t>(nullptr);
}

void PreparedStatementBase::SetValidData(const uint8 index, std::string_view value) // NOLINT(*-convert-member-functions-to-static)
{
    ASSERT(index < statement_data.size());
    statement_data[index].data.emplace<std::string>(value);
}

template void PreparedStatementBase::SetValidData(uint8 index, uint8 const& value);
template void PreparedStatementBase::SetValidData(uint8 index, int8 const& value);
template void PreparedStatementBase::SetValidData(uint8 index, uint16 const& value);
template void PreparedStatementBase::SetValidData(uint8 index, int16 const& value);
template void PreparedStatementBase::SetValidData(uint8 index, uint32 const& value);
template void PreparedStatementBase::SetValidData(uint8 index, int32 const& value);
template void PreparedStatementBase::SetValidData(uint8 index, uint64 const& value);
template void PreparedStatementBase::SetValidData(uint8 index, int64 const& value);
template void PreparedStatementBase::SetValidData(uint8 index, bool const& value);
template void PreparedStatementBase::SetValidData(uint8 index, float const& value);
template void PreparedStatementBase::SetValidData(uint8 index, std::string const& value);
template void PreparedStatementBase::SetValidData(uint8 index, std::vector<uint8> const& value);

PreparedStatementTask::PreparedStatementTask(PreparedStatementBase* stmt, const bool async) :
    m_stmt(stmt),
    m_result(nullptr)
{
    m_has_result = async; // If it's async, then there's a result

    if (async)
        m_result = new QueryResultPromise();
}

PreparedStatementTask::~PreparedStatementTask()
{
    delete m_stmt;

    if (m_has_result && m_result)
        delete m_result;
}

bool PreparedStatementTask::Execute()
{
    if (m_has_result)
    {
        ResultSet* result = m_conn->Query(m_stmt);
        if (!result || !result->GetRowCount())
        {
            delete result;
            m_result->set_value(QueryResult(nullptr));
            return false;
        }

        m_result->set_value(QueryResult(result));
        return true;
    }

    return m_conn->Execute(m_stmt);
}

template<typename T>
std::string PreparedStatementData::ToString(T value)
{
    return Acore::StringFormat("{}", value);
}

template<>
std::string PreparedStatementData::ToString(std::vector<uint8> /*value*/)
{
    return "BINARY";
}

template std::string PreparedStatementData::ToString(uint8);
template std::string PreparedStatementData::ToString(uint16);
template std::string PreparedStatementData::ToString(uint32);
template std::string PreparedStatementData::ToString(uint64);
template std::string PreparedStatementData::ToString(int8);
template std::string PreparedStatementData::ToString(int16);
template std::string PreparedStatementData::ToString(int32);
template std::string PreparedStatementData::ToString(int64);
template std::string PreparedStatementData::ToString(std::string);
template std::string PreparedStatementData::ToString(float);
template std::string PreparedStatementData::ToString(double);
template std::string PreparedStatementData::ToString(bool);

std::string PreparedStatementData::ToString(std::nullptr_t /*value*/)
{
    return "NULL";
}
