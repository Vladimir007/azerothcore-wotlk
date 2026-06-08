#ifndef NCORE_STRING_CONVERT_H
#define NCORE_STRING_CONVERT_H

#include <charconv>
#include <string>
#include <string_view>
#include <type_traits>

#include "Define.h"
#include "Errors.h"
#include "Optional.h"
#include "Types.h"
#include "Util.h"

namespace Acore::Impl::StringConvertImpl
{
    template <typename T, typename = void> struct For
    {
        static_assert(Acore::dependant_false_v<T>, "Unsupported type used for ToString or StringTo");
    };

    template <typename T>
    struct For<T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>>
    {
        static Optional<T> FromString(std::string_view str, int base = 10)
        {
            if (base == 0)
            {
                if (StringEqualI(str.substr(0, 2), "0x"))
                {
                    base = 16;
                    str.remove_prefix(2);
                }
                else if (StringEqualI(str.substr(0, 2), "0b"))
                {
                    base = 2;
                    str.remove_prefix(2);
                }
                else
                    base = 10;

                if (str.empty())
                    return std::nullopt;
            }

            char const* const start = str.data();
            char const* const end = (start + str.length());

            T val;
            std::from_chars_result const res = std::from_chars(start, end, val, base);
            if (res.ptr == end && res.ec == std::errc())
                return val;
            return std::nullopt;
        }

        // ReSharper disable once CppDFAConstantFunctionResult
        static std::string ToString(T val)
        {
            std::string buf(20, '\0'); /* 2^64 is 20 decimal characters, -(2^63) is 20 including the sign */
            char* const start = buf.data();
            char* const end = start + buf.length();
            std::to_chars_result const res = std::to_chars(start, end, val);
            ASSERT(res.ec == std::errc());
            buf.resize(res.ptr - start);
            return buf;
        }
    };

    template <>
    struct For<bool, void>
    {
        static Optional<bool> FromString(const std::string_view str, const int strict = 0)
        {
            if (strict)
            {
                if (str == "1")
                    return true;
                if (str == "0")
                    return false;
                return std::nullopt;
            }
            if (str == "1" || StringEqualI(str, "y") || StringEqualI(str, "on") || StringEqualI(str, "yes") || StringEqualI(str, "true"))
                return true;
            if (str == "0" || StringEqualI(str, "n") || StringEqualI(str, "off") || StringEqualI(str, "no") || StringEqualI(str, "false"))
                return false;
            return std::nullopt;
        }

        static std::string ToString(const bool val)
        {
            return val ? "1" : "0";
        }
    };

    template <typename T>
    struct For<T, std::enable_if_t<std::is_floating_point_v<T>>>
    {
        static Optional<T> FromString(const std::string_view str, const int base = 0)
        {
            try
            {
                if (str.empty())
                    return std::nullopt;

                if (base == 10 && StringEqualI(str.substr(0, 2), "0x"))
                    return std::nullopt;

                std::string tmp;
                if (base == 16)
                    tmp.append("0x");
                tmp.append(str);

                std::size_t n;
                T val = static_cast<T>(std::stold(tmp, &n));
                if (n != tmp.length())
                    return std::nullopt;
                return val;
            }
            catch (...)
            {
                return std::nullopt;
            }
        }

        static std::string ToString(T val)
        {
            return std::to_string(val);
        }
    };
}

namespace Acore
{
    template <typename Result, typename... Params>
    Optional<Result> StringTo(std::string_view str, Params&&... params)
    {
        return Impl::StringConvertImpl::For<Result>::FromString(str, std::forward<Params>(params)...);
    }

    template <typename Type, typename... Params>
    std::string ToString(Type&& val, Params&&... params)
    {
        return Impl::StringConvertImpl::For<std::decay_t<Type>>::ToString(std::forward<Type>(val), std::forward<Params>(params)...);
    }
}

#endif
