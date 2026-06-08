#ifndef FIELD_H
#define FIELD_H

#include <array>
#include <string_view>
#include <vector>
#include <pqxx/pqxx>

#include "Define.h"
#include "Duration.h"

namespace Acore::Types
{
    template <typename T>
    using is_chrono_v = std::enable_if_t<std::is_same_v<Milliseconds, T>
        || std::is_same_v<Seconds, T>
        || std::is_same_v<Minutes, T>
        || std::is_same_v<Hours, T>
        || std::is_same_v<Days, T>
        || std::is_same_v<Weeks, T>
        || std::is_same_v<Years, T>
        || std::is_same_v<Months, T>, T>;
}

using Binary = std::vector<uint8>;

class Field
{

public:
    Field() {}
    ~Field() = default;

    void Set(pqxx::field_ref ref);

    [[nodiscard]] bool IsNull() const { return _ref.is_null(); }

    template<typename T>
    std::enable_if_t<std::is_arithmetic_v<T> && !std::is_same_v<uint8, T> && !std::is_same_v<int8, T>, T> Get() const
    {
        return GetData<T>();
    }

    template<typename T>
    std::enable_if_t<std::is_same_v<uint8, T>, T> Get() const
    {
        return static_cast<uint8>(GetData<int>());
    }

    template<typename T>
    std::enable_if_t<std::is_same_v<int8, T>, T> Get() const
    {
        return static_cast<int8>(GetData<int>());
    }

    template<typename T>
    std::enable_if_t<std::is_same_v<std::string, T>, T> Get() const
    {
        return GetData<T>();
    }

    template<typename T>
    std::enable_if_t<std::is_same_v<std::string_view, T>, T> Get() const
    {
        return GetData<T>();
    }

    template<typename T>
    std::enable_if_t<std::is_same_v<Binary, T>, T> Get() const
    {
        return GetDataBinary();
    }

    template <typename T, std::size_t S>
    std::enable_if_t<std::is_same_v<Binary, T>, std::array<uint8, S>> Get() const
    {
        std::array<uint8, S> buf = {};
        GetBinarySizeChecked(buf.data(), S);
        return buf;
    }

    template<typename T>
    Acore::Types::is_chrono_v<T> Get(bool convertToUin32 = true) const
    {
        return convertToUin32 ? T(GetData<uint32>()) : T(GetData<uint64>());
    }

    template<typename T>
    std::vector<T> GetVector() const;

    template<typename T, std::size_t S>
    std::array<T, S> GetArray() const;

    /// Returns NxM array for T-values
    template<typename T, std::size_t N, std::size_t M>
    std::array<std::array<T, M>, N> GetArray() const;

private:
    template<typename T>
    T GetData() const;

    Binary GetDataBinary() const;
    void GetBinarySizeChecked(uint8* buf, std::size_t size) const;

    pqxx::field_ref _ref;
};

#endif
