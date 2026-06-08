#include "StringFormat.h"

template<class Str>
Str Acore::String::Trim(const Str& s, const std::locale& loc /*= std::locale()*/)
{
    typename Str::const_iterator first = s.begin();
    typename Str::const_iterator end = s.end();

    while (first != end && std::isspace(*first, loc))
        ++first;

    if (first == end)
        return Str();

    typename Str::const_iterator last = end;

    do
    {
        --last;
    } while (std::isspace(*last, loc));

    if (first != s.begin() || last + 1 != end)
        return Str(first, last + 1);

    return s;
}

// Template Trim
template std::string Acore::String::Trim<std::string>(const std::string& s, const std::locale& loc /*= std::locale()*/);
