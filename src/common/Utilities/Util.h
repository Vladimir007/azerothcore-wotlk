#ifndef NCORE_UTIL_H
#define NCORE_UTIL_H

#include <algorithm>
#include <array>
#include <cctype>
#include <list>
#include <map>
#include <string>

#include "Containers.h"
#include "Define.h"
#include "Errors.h"
#include "Optional.h"

// Searcher for map of structs
template<typename T, class S> struct Finder
{
    T val_;
    T S::* idMember_;

    Finder(T val, T S::* idMember) : val_(val), idMember_(idMember) {}
    bool operator()(const std::pair<int, S>& obj) { return obj.second.*idMember_ == val_; }
};

void stripLineInvisibleChars(std::string& src);

Optional<int32> MoneyStringToMoney(std::string_view moneyString);

std::string secsToTimeString(uint64 timeInSecs, bool shortText = false);
uint32 TimeStringToSecs(const std::string& timestring);

// Percentage calculation
template <class T, class U>
T CalculatePct(T base, U pct)
{
    return T(base * static_cast<float>(pct) / 100.0f);
}

template <class T, class U>
T AddPct(T& base, U pct)
{
    return base += CalculatePct(base, pct);
}

template <class T, class U>
T ApplyPct(T& base, U pct)
{
    return base = CalculatePct(base, pct);
}

template <class T>
T RoundToInterval(T& num, T floor, T ceil)
{
    return num = std::min<T>(std::max<T>(num, floor), ceil);
}

// UTF8 handling
bool Utf8toWStr(std::string_view utf8str, std::wstring& wstr);

// in wsize==max size of buffer, out wsize==real string size
bool Utf8toWStr(char const* utf8str, std::size_t csize, wchar_t* wstr, std::size_t& wsize);

inline bool Utf8toWStr(const std::string_view utf8str, wchar_t* wstr, std::size_t& wsize)
{
    return Utf8toWStr(utf8str.data(), utf8str.size(), wstr, wsize);
}

bool WStrToUtf8(std::wstring_view wstr, std::string& utf8str);

// size==real string size
bool WStrToUtf8(wchar_t const* wstr, std::size_t size, std::string& utf8str);

// set string to "" if invalid utf8 sequence
void utf8truncate(std::string& utf8str, std::size_t len);

inline bool isBasicLatinCharacter(const wchar_t wchar)
{
    if (wchar >= L'a' && wchar <= L'z')  // LATIN SMALL LETTER A - LATIN SMALL LETTER Z
        return true;
    if (wchar >= L'A' && wchar <= L'Z')  // LATIN CAPITAL LETTER A - LATIN CAPITAL LETTER Z
        return true;
    return false;
}

inline bool isExtendedLatinCharacter(const wchar_t wchar)
{
    if (isBasicLatinCharacter(wchar))
        return true;
    if (wchar >= 0x00C0 && wchar <= 0x00D6)  // LATIN CAPITAL LETTER A WITH GRAVE - LATIN CAPITAL LETTER O WITH DIAERESIS
        return true;
    if (wchar >= 0x00D8 && wchar <= 0x00DE)  // LATIN CAPITAL LETTER O WITH STROKE - LATIN CAPITAL LETTER THORN
        return true;
    if (wchar == 0x00DF)                     // LATIN SMALL LETTER SHARP S
        return true;
    if (wchar >= 0x00E0 && wchar <= 0x00F6)  // LATIN SMALL LETTER A WITH GRAVE - LATIN SMALL LETTER O WITH DIAERESIS
        return true;
    if (wchar >= 0x00F8 && wchar <= 0x00FE)  // LATIN SMALL LETTER O WITH STROKE - LATIN SMALL LETTER THORN
        return true;
    if (wchar >= 0x0100 && wchar <= 0x012F)  // LATIN CAPITAL LETTER A WITH MACRON - LATIN SMALL LETTER I WITH OGONEK
        return true;
    if (wchar == 0x1E9E)                     // LATIN CAPITAL LETTER SHARP S
        return true;
    return false;
}

inline bool isCyrillicCharacter(const wchar_t wchar)
{
    if (wchar >= 0x0410 && wchar <= 0x044F)  // CYRILLIC CAPITAL LETTER A - CYRILLIC SMALL LETTER YA
        return true;
    if (wchar == 0x0401 || wchar == 0x0451)  // CYRILLIC CAPITAL LETTER IO, CYRILLIC SMALL LETTER IO
        return true;
    return false;
}

inline bool isEastAsianCharacter(const wchar_t wchar)
{
    if (wchar >= 0x1100 && wchar <= 0x11F9) // Hangul Jamo
        return true;
    if (wchar >= 0x3041 && wchar <= 0x30FF) // Hiragana + Katakana
        return true;
    if (wchar >= 0x3131 && wchar <= 0x318E) // Hangul Compatibility Jamo
        return true;
    if (wchar >= 0x31F0 && wchar <= 0x31FF) // Katakana Phonetic Ext.
        return true;
    if (wchar >= 0x3400 && wchar <= 0x4DB5) // CJK Ideographs Ext. A
        return true;
    if (wchar >= 0x4E00 && wchar <= 0x9FC3) // Unified CJK Ideographs
        return true;
    if (wchar >= 0xAC00 && wchar <= 0xD7A3) // Hangul Syllables
        return true;
    if (wchar >= 0xFF01 && wchar <= 0xFFEE) // Halfwidth forms
        return true;
    return false;
}

inline bool isNumeric(const wchar_t wchar)
{
    return wchar >= L'0' && wchar <= L'9';
}

inline bool isNumeric(const char c)
{
    return c >= '0' && c <= '9';
}

inline bool IsEvenNumber(const int32 n)
{
    return n % 2 == 0;
}

inline bool isNumeric(char const* str)
{
    for (char const* c = str; *c; ++c)
        if (!isNumeric(*c))
            return false;

    return true;
}

inline bool isNumericOrSpace(const wchar_t wchar)
{
    return isNumeric(wchar) || wchar == L' ';
}

inline bool isBasicLatinString(const std::wstring_view wstr, const bool numericOrSpace)
{
    for (const wchar_t i : wstr)
        if (!isBasicLatinCharacter(i) && (!numericOrSpace || !isNumericOrSpace(i)))
            return false;
    return true;
}

inline bool isExtendedLatinString(const std::wstring_view wstr, const bool numericOrSpace)
{
    for (const wchar_t i : wstr)
        if (!isExtendedLatinCharacter(i) && (!numericOrSpace || !isNumericOrSpace(i)))
            return false;
    return true;
}

inline bool isCyrillicString(const std::wstring_view wstr, const bool numericOrSpace)
{
    for (const wchar_t i : wstr)
        if (!isCyrillicCharacter(i) && (!numericOrSpace || !isNumericOrSpace(i)))
            return false;
    return true;
}

inline bool isEastAsianString(const std::wstring_view wstr, const bool numericOrSpace)
{
    for (const wchar_t i : wstr)
        if (!isEastAsianCharacter(i) && (!numericOrSpace || !isNumericOrSpace(i)))
            return false;
    return true;
}

inline char charToUpper(const char c) { return std::toupper(c); }
inline char charToLower(const char c) { return std::tolower(c); }

inline wchar_t wcharToUpper(const wchar_t wchar)
{
    if (wchar >= L'a' && wchar <= L'z')                        // LATIN SMALL LETTER A - LATIN SMALL LETTER Z
        return static_cast<wchar_t>(static_cast<uint16>(wchar) - 0x0020);
    if (wchar == 0x00DF)                                       // LATIN SMALL LETTER SHARP S
        return 0x1E9E;
    if (wchar >= 0x00E0 && wchar <= 0x00F6)                    // LATIN SMALL LETTER A WITH GRAVE - LATIN SMALL LETTER O WITH DIAERESIS
        return static_cast<wchar_t>(static_cast<uint16>(wchar) - 0x0020);
    if (wchar >= 0x00F8 && wchar <= 0x00FE)                    // LATIN SMALL LETTER O WITH STROKE - LATIN SMALL LETTER THORN
        return static_cast<wchar_t>(static_cast<uint16>(wchar) - 0x0020);
    if (wchar >= 0x0101 && wchar <= 0x012F && wchar % 2 == 1)  // LATIN SMALL LETTER A WITH MACRON - LATIN SMALL LETTER I WITH OGONEK (only %2=1)
        return static_cast<wchar_t>(static_cast<uint16>(wchar) - 0x0001);
    if (wchar >= 0x0430 && wchar <= 0x044F)                    // CYRILLIC SMALL LETTER A - CYRILLIC SMALL LETTER YA
        return static_cast<wchar_t>(static_cast<uint16>(wchar) - 0x0020);
    if (wchar == 0x0451)                                       // CYRILLIC SMALL LETTER IO
        return 0x0401;
    return wchar;
}

inline wchar_t wcharToUpperOnlyLatin(const wchar_t wchar)
{
    return isBasicLatinCharacter(wchar) ? wcharToUpper(wchar) : wchar;
}

inline wchar_t wcharToLower(const wchar_t wchar)
{
    if (wchar >= L'A' && wchar <= L'Z')                       // LATIN CAPITAL LETTER A - LATIN CAPITAL LETTER Z
        return static_cast<wchar_t>(static_cast<uint16>(wchar) + 0x0020);
    if (wchar >= 0x00C0 && wchar <= 0x00D6)                   // LATIN CAPITAL LETTER A WITH GRAVE - LATIN CAPITAL LETTER O WITH DIAERESIS
        return static_cast<wchar_t>(static_cast<uint16>(wchar) + 0x0020);
    if (wchar >= 0x00D8 && wchar <= 0x00DE)                   // LATIN CAPITAL LETTER O WITH STROKE - LATIN CAPITAL LETTER THORN
        return static_cast<wchar_t>(static_cast<uint16>(wchar) + 0x0020);
    if (wchar >= 0x0100 && wchar <= 0x012E && wchar % 2 == 0) // LATIN CAPITAL LETTER A WITH MACRON - LATIN CAPITAL LETTER I WITH OGONEK (only %2=0)
        return static_cast<wchar_t>(static_cast<uint16>(wchar) + 0x0001);
    if (wchar == 0x1E9E)                                     // LATIN CAPITAL LETTER SHARP S
        return 0x00DF;
    if (wchar == 0x0401)                                     // CYRILLIC CAPITAL LETTER IO
        return 0x0451;
    if (wchar >= 0x0410 && wchar <= 0x042F)                  // CYRILLIC CAPITAL LETTER A - CYRILLIC CAPITAL LETTER YA
        return static_cast<wchar_t>(static_cast<uint16>(wchar) + 0x0020);
    return wchar;
}

void wstrToUpper(std::wstring& str);
void wstrToLower(std::wstring& str);

std::wstring GetMainPartOfName(std::wstring const& wName, uint32 declension);

bool Utf8FitTo(std::string_view str, std::wstring_view search);
void utf8printf(FILE* out, const char* str, ...);
void v_utf8printf(FILE* out, const char* str, const va_list* ap);
bool Utf8ToUpperOnlyLatin(std::string& utf8String);

namespace Acore::Impl
{
    std::string ByteArrayToHexStr(uint8 const* bytes, std::size_t length, bool reverse = false);
    void HexStrToByteArray(std::string_view str, uint8* out, std::size_t outLen, bool reverse = false);
}

template<typename Container>
std::string ByteArrayToHexStr(Container const& c, const bool reverse = false)
{
    return Acore::Impl::ByteArrayToHexStr(std::data(c), std::size(c), reverse);
}

template<std::size_t Size>
void HexStrToByteArray(const std::string_view str, std::array<uint8, Size>& buf, const bool reverse = false)
{
    Acore::Impl::HexStrToByteArray(str, buf.data(), Size, reverse);
}

template<std::size_t Size>
std::array<uint8, Size> HexStrToByteArray(std::string_view str, bool reverse = false)
{
    std::array<uint8, Size> arr;
    HexStrToByteArray(str, arr, reverse);
    return arr;
}

bool StringEqualI(std::string_view a, std::string_view b);
inline bool StringStartsWith(const std::string_view haystack, const std::string_view needle)
{
    return (haystack.substr(0, needle.length()) == needle);
}
inline bool StringStartsWithI(const std::string_view haystack, const std::string_view needle)
{
    return StringEqualI(haystack.substr(0, needle.length()), needle);
}
bool StringContainsStringI(std::string_view haystack, std::string_view needle);

template <typename T>
bool ValueContainsStringI(std::pair<T, std::string_view> const& haystack, const std::string_view needle)
{
    return StringContainsStringI(haystack.second, needle);
}

bool StringCompareLessI(std::string_view a, std::string_view b);

struct StringCompareLessI_T
{
    bool operator()(const std::string_view a, const std::string_view b) const { return StringCompareLessI(a, b); }
};

// Simple class for not-modifiable list
template <typename T>
class HookList
{
    typedef std::list<T>::iterator ListIterator;
    std::list<T> m_list;

public:
    HookList& operator+=(T t)
    {
        m_list.push_back(t);
        return *this;
    }

    HookList& operator-=(T t)
    {
        m_list.remove(t);
        return *this;
    }

    std::size_t size() { return m_list.size(); }
    ListIterator begin() { return m_list.begin(); }
    ListIterator end() { return m_list.end(); }
};

class flag96
{
    uint32 part[3];

public:
    /*implicit*/flag96(const uint32 p1 = 0, const uint32 p2 = 0, const uint32 p3 = 0)
    {
        part[0] = p1;
        part[1] = p2;
        part[2] = p3;
    }

    [[nodiscard]] bool IsEqual(const uint32 p1 = 0, const uint32 p2 = 0, const uint32 p3 = 0) const
    {
        return part[0] == p1 && part[1] == p2 && part[2] == p3;
    }

    [[nodiscard]] bool HasFlag(const uint32 p1 = 0, const uint32 p2 = 0, const uint32 p3 = 0) const
    {
        return part[0] & p1 || part[1] & p2 || part[2] & p3;
    }

    void Set(const uint32 p1 = 0, const uint32 p2 = 0, const uint32 p3 = 0)
    {
        part[0] = p1;
        part[1] = p2;
        part[2] = p3;
    }

    bool operator<(flag96 const& right) const
    {
        for (uint8 i = 3; i > 0; --i)
        {
            if (part[i - 1] < right.part[i - 1])
                return true;
            if (part[i - 1] > right.part[i - 1])
                return false;
        }
        return false;
    }

    bool operator==(flag96 const& right) const
    {
        return part[0] == right.part[0] && part[1] == right.part[1] && part[2] == right.part[2];
    }

    bool operator!=(flag96 const& right) const
    {
        return !(*this == right);
    }

    flag96& operator=(flag96 const& right)
    {
        part[0] = right.part[0];
        part[1] = right.part[1];
        part[2] = right.part[2];
        return *this;
    }
    flag96(const flag96&) = default;
    flag96(flag96&&) = default;

    flag96 operator&(flag96 const& right) const
    {
        return flag96(part[0] & right.part[0], part[1] & right.part[1], part[2] & right.part[2]);
    }

    flag96& operator&=(flag96 const& right)
    {
        part[0] &= right.part[0];
        part[1] &= right.part[1];
        part[2] &= right.part[2];
        return *this;
    }

    flag96 operator|(flag96 const& right) const
    {
        return flag96(part[0] | right.part[0], part[1] | right.part[1], part[2] | right.part[2]);
    }

    flag96& operator|=(flag96 const& right)
    {
        part[0] |= right.part[0];
        part[1] |= right.part[1];
        part[2] |= right.part[2];
        return *this;
    }

    flag96 operator~() const
    {
        return flag96(~part[0], ~part[1], ~part[2]);
    }

    flag96 operator^(flag96 const& right) const
    {
        return flag96(part[0] ^ right.part[0], part[1] ^ right.part[1], part[2] ^ right.part[2]);
    }

    flag96& operator^=(flag96 const& right)
    {
        part[0] ^= right.part[0];
        part[1] ^= right.part[1];
        part[2] ^= right.part[2];
        return *this;
    }

    explicit operator bool() const
    {
        return part[0] != 0 || part[1] != 0 || part[2] != 0;
    }

    bool operator !() const
    {
        return !static_cast<bool>(*this);
    }

    uint32& operator[](const uint8 el)
    {
        return part[el];
    }

    uint32 const& operator [](const uint8 el) const
    {
        return part[el];
    }
};

enum ComparisonType
{
    COMP_TYPE_EQ = 0,
    COMP_TYPE_HIGH,
    COMP_TYPE_LOW,
    COMP_TYPE_HIGH_EQ,
    COMP_TYPE_LOW_EQ,
    COMP_TYPE_MAX
};

template <class T>
bool CompareValues(const ComparisonType type, T val1, T val2)
{
    switch (type)
    {
        case COMP_TYPE_EQ:
            return val1 == val2;
        case COMP_TYPE_HIGH:
            return val1 > val2;
        case COMP_TYPE_LOW:
            return val1 < val2;
        case COMP_TYPE_HIGH_EQ:
            return val1 >= val2;
        case COMP_TYPE_LOW_EQ:
            return val1 <= val2;
        default:
            // Incorrect parameter
            ABORT();
            return false;
    }
}

template<typename E>
constexpr std::underlying_type_t<E> AsUnderlyingType(E enumValue)
{
    static_assert(std::is_enum_v<E>, "AsUnderlyingType can only be used with enums");
    return static_cast<std::underlying_type_t<E>>(enumValue);
}

template<typename Ret, typename T1, typename... T>
Ret* Coalesce(T1* first, T*... rest)
{
    if constexpr (sizeof...(T) > 0)
        return first ? static_cast<Ret*>(first) : Coalesce<Ret>(rest...);
    else
        return static_cast<Ret*>(first);
}

std::string GetTypeName(std::type_info const&);

template <typename T>
std::string GetTypeName() { return GetTypeName(typeid(T)); }

template <typename T>
std::enable_if_t<!std::is_same_v<std::decay_t<T>, std::type_info>, std::string> GetTypeName([[maybe_unused]] T&& v) { return GetTypeName(typeid(v)); }

#endif
