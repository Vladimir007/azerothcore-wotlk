#include "Field.h"

#include "Errors.h"
#include "StringConvert.h"

void Field::Set(const pqxx::field_ref ref)
{
    _ref = ref;
}

namespace
{
    template<typename T>
    constexpr T GetDefaultValue()
    {
        if constexpr (std::is_same_v<T, bool>)
            return false;
        else if constexpr (std::is_integral_v<T>)
            return 0;
        else if constexpr (std::is_floating_point_v<T>)
            return 1.0f;
        else if constexpr (std::is_same_v<T, std::vector<uint8>> || std::is_same_v<std::string_view, T>)
            return {};
        else
            return "";
    }
}

template<typename T>
T Field::GetData() const
{
    static_assert(std::is_arithmetic_v<T>, "Unsupported type for Field::GetData()");

    if (IsNull())
        return GetDefaultValue<T>();

    if constexpr (std::is_same_v<T, uint8>)
        return static_cast<uint8>(_ref.as<int>());
    else if constexpr (std::is_same_v<T, int8>)
        return static_cast<int8>(_ref.as<int>());
    return _ref.as<T>();
}

template bool Field::GetData() const;
template uint16 Field::GetData() const;
template uint32 Field::GetData() const;
template uint64 Field::GetData() const;
template int16 Field::GetData() const;
template int32 Field::GetData() const;
template int64 Field::GetData() const;
template float Field::GetData() const;
template double Field::GetData() const;

Binary Field::GetDataBinary() const
{
    if (IsNull())
        return {};

    const auto res = _ref.as<std::vector<std::byte>>();

    Binary result;
    result.resize(res.size());
    memcpy(result.data(), res.data(), res.size());
    return result;
}

void Field::GetBinarySizeChecked(uint8* buf, std::size_t size) const
{
    const auto _data = IsNull() ? std::vector<std::byte>{} : _ref.as<std::vector<std::byte>>();
    ASSERT(_data.size() == size, "Expected {}-byte binary blob, got {}data ({} bytes) instead", size, _data.size() ? "" : "no ", _data.size());
    memcpy(buf, _data.data(), size);
}

template<typename T>
std::vector<T> Field::GetVector() const
{
    if (IsNull())
        return std::vector<T>();

    if constexpr (std::is_same_v<T, uint8> || std::is_same_v<T, int8>)
    {
        std::vector<T> result;
        for (const auto _data = _ref.as<std::vector<int>>(); auto item : _data)
            result.push_back(static_cast<T>(item));
        return result;
    }

    return _ref.as<std::vector<T>>();
}

template<typename T, std::size_t S>
std::array<T, S> Field::GetArray() const
{
    std::array<T, S> result = {};

    ASSERT(!IsNull(), "Expected {}-size vector, got NULL instead", S);
    if (IsNull())
        return result;

    if constexpr (std::is_same_v<T, uint8> || std::is_same_v<T, int8>)
    {
        const auto _data = _ref.as<std::vector<int>>();
        ASSERT(_data.size() == S, "Expected {}-size vector, got {} instead", S, _data.size());
        for (int i = 0; i < S; ++i)
            result[i] = static_cast<T>(_data[i]);
        return result;
    }

    const std::vector<T> _data = _ref.as<std::vector<T>>();
    ASSERT(_data.size() == S, "Expected {}-size vector, got {} instead", S, _data.size());
    memcpy(result.data(), _data.data(), sizeof(result));
    return result;
}

template<typename T, std::size_t N, std::size_t M>
std::array<std::array<T, M>, N> Field::GetArray() const
{
    std::array<std::array<T, M>, N> result = {};

    ASSERT(!IsNull(), "Expected {}x{} matrix, got NULL instead", N, M);
    if (IsNull())
        return result;

    if constexpr (std::is_same_v<T, uint8> || std::is_same_v<T, int8>)
    {
        const auto _data = _ref.as<pqxx::array<int, 2>>();
        for (std::size_t i = 0; i < N; ++i) {
            for (std::size_t j = 0; j < M; ++j) {
                result[i][j] = static_cast<T>(_data.at(i, j));
            }
        }
        return result;
    }

    const auto _data = _ref.as<pqxx::array<T, 2>>();
    for (std::size_t i = 0; i < N; ++i) {
        for (std::size_t j = 0; j < M; ++j) {
            result[i][j] = _data.at(i, j);
        }
    }
    return result;
}