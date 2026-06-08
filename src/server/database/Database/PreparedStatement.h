#ifndef PREPARED_STATEMENT_H
#define PREPARED_STATEMENT_H

#include <chrono>
#include <future>
#include <tuple>
#include <variant>
#include <vector>
#include "Define.h"
#include "SQLOperation.h"

namespace Acore::Types
{
    template <typename T>
    using is_default = std::enable_if_t<std::is_arithmetic_v<T> || std::is_same_v<std::vector<uint8>, T>>;

    template <typename T>
    using is_enum_v = std::enable_if_t<std::is_enum_v<T>>;

    template <typename T>
    using is_non_string_view_v = std::enable_if_t<!std::is_base_of_v<std::string_view, T>>;
}

struct PreparedStatementData
{
    std::variant<
        bool,
        uint8,
        uint16,
        uint32,
        uint64,
        int8,
        int16,
        int32,
        int64,
        float,
        double,
        std::string,
        std::vector<uint8>,
        std::nullptr_t
    > data;

    template<typename T>
    static std::string ToString(T value);

    static std::string ToString(std::nullptr_t /*value*/);
};

//- Upper-level class that is used in code
class PreparedStatementBase
{
friend class PreparedStatementTask;

public:
    explicit PreparedStatementBase(uint32 index, uint8 capacity);
    virtual ~PreparedStatementBase();

    // Set numeric and default binary
    template<typename T>
    Acore::Types::is_default<T> SetData(const uint8 index, T value)
    {
        SetValidData(index, value);
    }

    // Set enums
    template<typename T>
    Acore::Types::is_enum_v<T> SetData(const uint8 index, T value)
    {
        SetValidData(index, std::underlying_type_t<T>(value));
    }

    // Set string_view
    void SetData(const uint8 index, const std::string_view value)
    {
        SetValidData(index, value);
    }

    // Set nullptr
    void SetData(const uint8 index, std::nullptr_t = nullptr)
    {
        SetValidData(index);
    }

    // Set non default binary
    template<std::size_t Size>
    void SetData(const uint8 index, std::array<uint8, Size> const& value)
    {
        const std::vector<uint8> vec(value.begin(), value.end());
        SetValidData(index, vec);
    }

    // Set duration
    template<class _Rep, class _Period>
    void SetData(const uint8 index, std::chrono::duration<_Rep, _Period> const& value, bool convertToUin32 = true)
    {
        SetValidData(index, convertToUin32 ? static_cast<uint32>(value.count()) : value.count());
    }

    // Set all
    template <typename... Args>
    void SetArguments(Args&&... args)
    {
        SetDataTuple(std::make_tuple(std::forward<Args>(args)...));
    }

    [[nodiscard]] uint32 GetIndex() const { return m_index; }
    [[nodiscard]] std::vector<PreparedStatementData> const& GetParameters() const { return statement_data; }

protected:
    template<typename T>
    Acore::Types::is_non_string_view_v<T> SetValidData(uint8 index, T const& value);

    void SetValidData(uint8 index);
    void SetValidData(uint8 index, std::string_view value);

    template<typename... Ts>
    void SetDataTuple(std::tuple<Ts...> const& argsList)
    {
        std::apply
        (
            [this](Ts const&... arguments)
            {
                uint8 index{ 0 };
                // ReSharper disable once CppDFAUnusedValue
                ((SetData(index, arguments), index++), ...);
            }, argsList
        );
    }

    uint32 m_index;

    //- Buffer of parameters, not tied to PostgreSQL in any way yet
    std::vector<PreparedStatementData> statement_data{};

    PreparedStatementBase(PreparedStatementBase const& right) = delete;
    PreparedStatementBase& operator=(PreparedStatementBase const& right) = delete;
};

template<typename>
class PreparedStatement : public PreparedStatementBase
{
public:
    explicit PreparedStatement(const uint32 index, const uint8 capacity) : PreparedStatementBase(index, capacity) {}

private:
    PreparedStatement(PreparedStatement const& right) = delete;
    PreparedStatement& operator=(PreparedStatement const& right) = delete;
};

class PreparedStatementTask : public SQLOperation
{
public:
    explicit PreparedStatementTask(PreparedStatementBase* stmt, bool async = false);
    ~PreparedStatementTask() override;

    bool Execute() override;
    QueryResultFuture GetFuture() const { return m_result->get_future(); }

protected:
    PreparedStatementBase* m_stmt;
    bool m_has_result;
    QueryResultPromise* m_result;
};

#endif
