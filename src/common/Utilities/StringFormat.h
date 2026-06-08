#ifndef STRING_FORMAT_H
#define STRING_FORMAT_H

#include <locale>
#include <format>
#include <string>

template <typename T>
requires std::is_enum_v<T>
struct std::formatter<T> : std::formatter<std::underlying_type_t<T>> {
    auto format(T e, format_context& ctx) const {
        return std::formatter<std::underlying_type_t<T>>::format(
            static_cast<std::underlying_type_t<T>>(e), ctx);
    }
};

namespace Acore
{
    /// Default string format function.
    template<typename... Args>
    std::string StringFormat(const std::string_view fmt, Args&&... args)
    {
        try
        {
            return std::vformat(fmt, std::make_format_args(args...));
        }
        catch (const std::exception& e)
        {
            return std::format("Wrong format occurred ({}). Format string: '{}'", e.what(), std::string_view(fmt));
        }
    }
}

namespace Acore::String
{
    template<class Str>
    Str Trim(const Str& s, const std::locale& loc = std::locale());
}

#endif
