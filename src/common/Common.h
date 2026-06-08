#ifndef NCORE_COMMON_H
#define NCORE_COMMON_H

#include <string>
#include <cstdlib>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "Define.h"

#define STRINGIZE(a) #a

#define MAX_NET_CLIENT_PACKET_SIZE (32767 - 1)  // Client hard cap: int16 with trailing zero space otherwise crash on memory free

// TimeConstants
constexpr auto SECOND = 1;
constexpr auto MINUTE = SECOND * 60;
constexpr auto HOUR = MINUTE * 60;
constexpr auto DAY = HOUR * 24;
constexpr auto WEEK = DAY * 7;
constexpr auto MONTH = DAY * 30;
constexpr auto YEAR = DAY * 365;
constexpr auto IN_MILLISECONDS = 1000;

enum AccountTypes
{
    SEC_PLAYER         = 0,
    SEC_MODERATOR      = 1,
    SEC_GAME_MASTER    = 2,
    SEC_ADMINISTRATOR  = 3,
    SEC_CONSOLE        = 4                                  // must be always last in list, accounts must have less security level always also
};

constexpr uint32 ACCOUNT_FLAG_GM = 0x1;

enum LocaleConstant
{
    LOCALE_enUS = 0,
    LOCALE_koKR = 1,
    LOCALE_frFR = 2,
    LOCALE_deDE = 3,
    LOCALE_zhCN = 4,
    LOCALE_zhTW = 5,
    LOCALE_esES = 6,
    LOCALE_esMX = 7,
    LOCALE_ruRU = 8,

    TOTAL_LOCALES
};

#define DEFAULT_LOCALE LOCALE_enUS

#define MAX_LOCALES 8
#define MAX_ACCOUNT_TUTORIAL_VALUES 8

extern char const* localeNames[TOTAL_LOCALES];

bool IsLocaleValid(const std::string& locale);
LocaleConstant GetLocaleByName(const std::string& name);
std::string GetNameByLocaleConstant(LocaleConstant localeConstant);

namespace Acore
{
    template<class ArgumentType, class ResultType>
    struct unary_function
    {
        typedef ArgumentType argument_type;
        typedef ResultType result_type;
    };
}

#endif
