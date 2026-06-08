#include "Common.h"

char const* localeNames[TOTAL_LOCALES] =
{
    "enUS",
    "koKR",
    "frFR",
    "deDE",
    "zhCN",
    "zhTW",
    "esES",
    "esMX",
    "ruRU"
};

bool IsLocaleValid(std::string const& locale)
{
    for (int i = 0; i < TOTAL_LOCALES; ++i)
        if (locale == localeNames[i])
            return true;
    return false;
}

LocaleConstant GetLocaleByName(const std::string& name)
{
    for (uint32 i = 0; i < TOTAL_LOCALES; ++i)
        if (name == localeNames[i])
            return static_cast<LocaleConstant>(i);
    return LOCALE_enUS;
}

std::string GetNameByLocaleConstant(const LocaleConstant localeConstant)
{
    if (localeConstant < TOTAL_LOCALES)
        return localeNames[localeConstant];
    return "enUS";
}
