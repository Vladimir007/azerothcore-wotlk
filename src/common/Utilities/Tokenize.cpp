#include "Tokenize.h"

std::vector<std::string_view> Acore::Tokenize(std::string_view str, const char sep, const bool keepEmpty)
{
    std::vector<std::string_view> tokens;

    std::size_t start = 0;
    for (std::size_t end = str.find(sep); end != std::string_view::npos; end = str.find(sep, start))
    {
        if (keepEmpty || start < end)
            tokens.push_back(str.substr(start, end - start));

        start = end + 1;
    }

    if (keepEmpty || start < str.length())
        tokens.push_back(str.substr(start));

    return tokens;
}
