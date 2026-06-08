#ifndef NCORE_TOKENIZE_H
#define NCORE_TOKENIZE_H

#include <string_view>
#include <vector>

namespace Acore
{
    std::vector<std::string_view> Tokenize(std::string_view str, char sep, bool keepEmpty);

    /* This would return string_view into temporary otherwise */
    std::vector<std::string_view> Tokenize(std::string&&, char, bool) = delete;
    std::vector<std::string_view> Tokenize(std::string const&&, char, bool) = delete;

    /* The delete overload means we need to make this explicit */
    inline std::vector<std::string_view> Tokenize(char const* str, const char sep, const bool keepEmpty)
    {
        return Tokenize(std::string_view(str ? str : ""), sep, keepEmpty);
    }
}

#endif
