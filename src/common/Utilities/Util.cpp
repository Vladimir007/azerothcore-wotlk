#include "Util.h"

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <sstream>
#include <string>
#include <utf8.h>
#include <boost/core/demangle.hpp>

#include "Common.h"
#include "Containers.h"
#include "IpAddress.h"
#include "StringConvert.h"
#include "Tokenize.h"

void stripLineInvisibleChars(std::string& src)
{
    static constexpr std::string invChars = " \t\7\n";

    std::size_t wpos = 0;

    bool space = false;
    for (std::size_t pos = 0; pos < src.size(); ++pos)
    {
        if (invChars.find(src[pos]) != std::string::npos)
        {
            if (!space)
            {
                src[wpos++] = ' ';
                space = true;
            }
        }
        else
        {
            if (wpos != pos)
                src[wpos++] = src[pos];
            else
                ++wpos;
            space = false;
        }
    }

    if (wpos < src.size())
        src.erase(wpos, src.size());
    if (src.find("|TInterface") != std::string::npos)
        src.clear();
}

std::string secsToTimeString(const uint64 timeInSecs, const bool shortText)
{
    const uint64 secs    = timeInSecs % MINUTE;
    const uint64 minutes = timeInSecs % HOUR / MINUTE;
    const uint64 hours   = timeInSecs % DAY  / HOUR;
    const uint64 days    = timeInSecs / DAY;

    std::ostringstream ss;
    if (days)
        ss << days << (shortText ? "d" : " day(s) ");
    if (hours)
        ss << hours << (shortText ? "h" : " hour(s) ");
    if (minutes)
        ss << minutes << (shortText ? "m" : " minute(s) ");
    if (secs || (!days && !hours && !minutes))
        ss << secs << (shortText ? "s" : " second(s) ");

    std::string str = ss.str();

    if (!shortText && !str.empty() && str[str.size() - 1] == ' ')
        str.resize(str.size() - 1);

    return str;
}

Optional<int32> MoneyStringToMoney(const std::string_view moneyString)
{
    int32 money = 0;

    bool hadG = false;
    bool hadS = false;
    bool hadC = false;

    for (std::string_view token : Acore::Tokenize(moneyString, ' ', false))
    {
        uint32 unit;
        switch (token[token.length() - 1])
        {
        case 'g':
            if (hadG)
            {
                return std::nullopt;
            }
            hadG = true;
            unit = 100 * 100;
            break;
        case 's':
            if (hadS)
            {
                return std::nullopt;
            }
            hadS = true;
            unit = 100;
            break;
        case 'c':
            if (hadC)
            {
                return std::nullopt;
            }
            hadC = true;
            unit = 1;
            break;
        default:
            return std::nullopt;
        }

        if (Optional<uint32> amount = Acore::StringTo<uint32>(token.substr(0, token.length() - 1)))
            money += unit * *amount;
        else
            return std::nullopt;
    }

    return money;
}

uint32 TimeStringToSecs(const std::string& timestring)
{
    uint32 secs       = 0;
    uint32 buffer     = 0;
    uint32 multiplier = 0;

    for (auto itr = timestring.begin(); itr != timestring.end(); ++itr)
    {
        if (isdigit(*itr))
        {
            buffer *= 10;
            buffer += (*itr) - '0';
        }
        else
        {
            switch (*itr)
            {
                case 'd':
                    multiplier = DAY;
                    break;
                case 'h':
                    multiplier = HOUR;
                    break;
                case 'm':
                    multiplier = MINUTE;
                    break;
                case 's':
                    multiplier = 1;
                    break;
                default :
                    return 0;  // Bad format
            }
            buffer *= multiplier;
            secs += buffer;
            buffer = 0;
        }
    }

    return secs;
}

void utf8truncate(std::string& utf8str, const std::size_t len)
{
    try
    {
        const std::size_t wlen = utf8::distance(utf8str.c_str(), utf8str.c_str() + utf8str.size());
        if (wlen <= len)
            return;

        std::wstring wstr;
        wstr.resize(wlen);
        utf8::utf8to16(utf8str.c_str(), utf8str.c_str() + utf8str.size(), &wstr[0]);
        wstr.resize(len);
        const char* oEnd = utf8::utf16to8(wstr.c_str(), wstr.c_str() + wstr.size(), &utf8str[0]);
        utf8str.resize(oEnd - &utf8str[0]);  // Remove unused tail
    }
    catch (std::exception const&)
    {
        utf8str.clear();
    }
}

bool Utf8toWStr(char const* utf8str, const std::size_t csize, wchar_t* wstr, std::size_t& wsize)
{
    try
    {
        Acore::CheckedBufferOutputIterator out(wstr, wsize);
        out = utf8::utf8to16(utf8str, utf8str + csize, out);
        wsize -= out.remaining(); // Remaining unused space
        wstr[wsize] = L'\0';
    }
    catch (std::exception const&)
    {
        // Replace the converted string with an error message if there is enough space
        // Otherwise just return an empty string
        const auto errorMessage = L"An error occurred converting string from UTF-8 to WStr";
        const std::size_t errorMessageLength = std::char_traits<wchar_t>::length(errorMessage);
        if (wsize >= errorMessageLength)
        {
            std::wcscpy(wstr, errorMessage);
            wsize = std::char_traits<wchar_t>::length(wstr);
        }
        else if (wsize > 0)
        {
            wstr[0] = L'\0';
            wsize = 0;
        }
        else
            wsize = 0;

        return false;
    }

    return true;
}

bool Utf8toWStr(const std::string_view utf8str, std::wstring& wstr)
{
    wstr.clear();
    try
    {
        utf8::utf8to16(utf8str.begin(), utf8str.end(), std::back_inserter(wstr));
    }
    catch (std::exception const&)
    {
        wstr.clear();
        return false;
    }

    return true;
}

bool WStrToUtf8(wchar_t const* wstr, const std::size_t size, std::string& utf8str)
{
    try
    {
        std::string utf8str2;
        utf8str2.resize(size * 4);  // Allocate for most long case

        if (size)
        {
            const char* oEnd = utf8::utf16to8(wstr, wstr + size, &utf8str2[0]);
            utf8str2.resize(oEnd - &utf8str2[0]);  // Remove unused tail
        }

        utf8str = utf8str2;
    }
    catch (std::exception const&)
    {
        utf8str.clear();
        return false;
    }

    return true;
}

bool WStrToUtf8(const std::wstring_view wstr, std::string& utf8str)
{
    try
    {
        std::string utf8str2;
        utf8str2.resize(wstr.size() * 4);  // Allocate for most long case

        if (!wstr.empty())
        {
            const char* oEnd = utf8::utf16to8(wstr.begin(), wstr.end(), &utf8str2[0]);
            utf8str2.resize(oEnd - &utf8str2[0]);  // Remove unused tail
        }

        utf8str = utf8str2;
    }
    catch (std::exception const&)
    {
        utf8str.clear();
        return false;
    }

    return true;
}

void wstrToUpper(std::wstring& str) { std::ranges::transform(str, std::begin(str), wcharToUpper); }
void wstrToLower(std::wstring& str) { std::ranges::transform(str, std::begin(str), wcharToLower); }
void strToUpper(std::string& str) { std::ranges::transform(str, std::begin(str), charToUpper); }
void strToLower(std::string& str) { std::ranges::transform(str, std::begin(str), charToLower); }

std::wstring GetMainPartOfName(std::wstring const& wName, const uint32_t declension)
{
    // Supported only Cyrillic cases
    if (wName.empty() || !isCyrillicCharacter(wName[0]) || declension > 5)
        return wName;

    // Important: end length must be <= MAX_INTERNAL_PLAYER_NAME-MAX_PLAYER_NAME (3 currently)
    static constexpr std::wstring a_End     = L"\u0430";
    static constexpr std::wstring o_End     = L"\u043E";
    static constexpr std::wstring ya_End    = L"\u044F";
    static constexpr std::wstring ie_End    = L"\u0435";
    static constexpr std::wstring i_End     = L"\u0438";
    static constexpr std::wstring ye_ru_End = L"\u044B";
    static constexpr std::wstring u_End     = L"\u0443";
    static constexpr std::wstring yu_End    = L"\u044E";
    static constexpr std::wstring oj_End    = L"\u043E\u0439";
    static constexpr std::wstring ie_j_End  = L"\u0435\u0439";
    static constexpr std::wstring io_j_End  = L"\u0451\u0439";
    static constexpr std::wstring o_m_End   = L"\u043E\u043C";
    static constexpr std::wstring io_m_End  = L"\u0451\u043C";
    static constexpr std::wstring ie_m_End  = L"\u0435\u043C";
    static constexpr std::wstring soft_End  = L"\u044C";
    static constexpr std::wstring j_End     = L"\u0439";

    static std::array<std::array<std::wstring const*, 7>, 6> const dropEnds = {{
            { &a_End,  &o_End,    &ya_End,   &ie_End,  &soft_End, &j_End,    nullptr },
            { &a_End,  &ya_End,   &ye_ru_End, &i_End,   nullptr,   nullptr,   nullptr },
            { &ie_End, &u_End,    &yu_End,   &i_End,   nullptr,   nullptr,   nullptr },
            { &u_End,  &yu_End,   &o_End,    &ie_End,  &soft_End, &ya_End,   &a_End  },
            { &oj_End, &io_j_End, &ie_j_End, &o_m_End, &io_m_End, &ie_m_End, &yu_End },
            { &ie_End, &i_End,    nullptr,   nullptr,  nullptr,   nullptr,   nullptr }
        }
    };

    std::size_t const thisLen = wName.length();
    std::array<std::wstring const*, 7> const& endings = dropEnds[declension];
    for (const std::wstring* endingPtr : endings)
    {
        if (endingPtr == nullptr)
            break;

        std::wstring const& ending = *endingPtr;
        std::size_t const endLen = ending.length();
        if (endLen > thisLen)
            continue;

        if (wName.compare(thisLen - endLen, endLen, ending) == 0)
            return wName.substr(0, thisLen - endLen);
    }

    return wName;
}

bool Utf8FitTo(const std::string_view str, const std::wstring_view search)
{
    std::wstring temp;

    if (!Utf8toWStr(str, temp))
        return false;

    // Converting to lower case
    wstrToLower(temp);

    if (temp.find(search) == std::wstring::npos)
        return false;
    return true;
}

void utf8printf(FILE* out, const char* str, ...)
{
    va_list ap;
    va_start(ap, str);
    v_utf8printf(out, str, &ap);
    va_end(ap);
}

void v_utf8printf(FILE* out, const char* str, va_list* ap)
{
    vfprintf(out, str, *ap);
}

bool Utf8ToUpperOnlyLatin(std::string& utf8String)
{
    std::wstring wstr;
    if (!Utf8toWStr(utf8String, wstr))
        return false;

    std::ranges::transform(wstr, wstr.begin(), wcharToUpperOnlyLatin);
    return WStrToUtf8(wstr, utf8String);
}

std::string Acore::Impl::ByteArrayToHexStr(uint8 const* bytes, const std::size_t length, const bool reverse /* = false */)
{
    int32 init = 0;
    int32 end = length;
    int8 op = 1;

    if (reverse)
    {
        init = length - 1;
        end = -1;
        op = -1;
    }

    std::ostringstream ss;
    for (int32 i = init; i != end; i += op)
    {
        char buffer[4];
        snprintf(buffer, sizeof(buffer), "%02X", bytes[i]);
        ss << buffer;
    }

    return ss.str();
}

void Acore::Impl::HexStrToByteArray(const std::string_view str, uint8* out, const std::size_t outLen, const bool reverse /*= false*/)
{
    ASSERT(str.size() == 2 * outLen);

    int32 init = 0;
    int32 end = static_cast<int32>(str.length());
    int8 op = 1;

    if (reverse)
    {
        init = static_cast<int32>(str.length() - 2);
        end = -2;
        op = -1;
    }

    uint32 j = 0;
    for (int32 i = init; i != end; i += 2 * op)
    {
        const char buffer[3] = { str[i], str[i + 1], '\0' };
        out[j++] = static_cast<uint8>(strtoul(buffer, nullptr, 16));
    }
}

bool StringEqualI(const std::string_view a, const std::string_view b)
{
    return std::ranges::equal(a, b, [](const char c1, const char c2) { return std::tolower(c1) == std::tolower(c2); });
}

bool StringContainsStringI(const std::string_view haystack, const std::string_view needle)
{
    return haystack.end() !=
        std::ranges::search(haystack, needle, [](const char c1, const char c2) { return std::tolower(c1) == std::tolower(c2); }).begin();
}

bool StringCompareLessI(std::string_view a, std::string_view b)
{
    return std::ranges::lexicographical_compare(a, b, [](const char c1, const char c2) { return std::tolower(c1) < std::tolower(c2); });
}

std::string GetTypeName(std::type_info const& info)
{
    return boost::core::demangle(info.name());
}
