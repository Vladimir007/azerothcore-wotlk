#include "PostgreSQLPreparedStatement.h"

#include <string_view>
#include <algorithm>
#include <charconv>
#include <pqxx/pqxx>
#include <regex>

#include "Errors.h"
#include "PreparedStatement.h"

PSQLPreparedStatement::PSQLPreparedStatement(const std::string_view name, const std::string_view queryString):
    m_name(std::string(name)), m_queryString(std::string(queryString))
{
    m_paramCount = ExtractParamCount(queryString);
}

uint32 PSQLPreparedStatement::ExtractParamCount(const std::string_view sql)
{
    uint32 maxVal = 0;

    size_t pos = sql.find('$');
    while (pos != std::string_view::npos) {
        std::string_view sub = sql.substr(pos + 1);

        uint32 currentVal = 0;
        auto [ptr, ec] = std::from_chars(sub.data(), sub.data() + sub.size(), currentVal);
        if (ec == std::errc())
            maxVal = std::max(maxVal, currentVal);
        const size_t next_start = ptr - sql.data();
        pos = sql.find('$', next_start);
    }
    return maxVal;
}

pqxx::params PSQLPreparedStatement::BindParameters(const PreparedStatementBase* stmt)
{
    pqxx::params params;
    for (const auto& [data] : stmt->GetParameters())
    {
        std::visit([&](auto&& param)
        {
            AddParameter(param, params);
        }, data);
    }
    return params;
}

template<typename T>
void PSQLPreparedStatement::AddParameter(T value, pqxx::params& params)
{
    params.append(value);
}

void PSQLPreparedStatement::AddParameter(const int8 value, pqxx::params& params)
{
    AddParameter(static_cast<int>(value), params);
}

void PSQLPreparedStatement::AddParameter(const uint8 value, pqxx::params& params)
{
    AddParameter(static_cast<int>(value), params);
}

void PSQLPreparedStatement::AddParameter(std::vector<uint8> const& value, pqxx::params& params)
{
    params.append(pqxx::binary_cast(value));
}
