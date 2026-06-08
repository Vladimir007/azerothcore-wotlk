#include "Timer.h"
#include <iomanip>
#include <sstream>

std::tm Acore::Time::TimeBreakdown(time_t t /*= 0*/)
{
    if (!t)
        t = GetEpochTime().count();

    std::tm timeLocal;
    localtime_r(&t, &timeLocal);
    return timeLocal;
}

std::string Acore::Time::TimeToTimestampStr(const Seconds time /*= 0s*/, const std::string_view fmt /*= {}*/)
{
    std::stringstream ss;
    std::string format{ fmt };
    const time_t t = time.count();

    if (format.empty())
        format = "%Y-%m-%d %X";

    ss << std::put_time(std::localtime(&t), format.c_str());
    return ss.str();
}

std::string Acore::Time::TimeToHumanReadable(const Seconds time /*= 0s*/, const std::string_view fmt /*= {}*/)
{
    std::stringstream ss;
    std::string format{ fmt };
    const time_t t = time.count();

    if (format.empty())
        format = "%a %b %d %Y %X";

    ss << std::put_time(std::localtime(&t), format.c_str());
    return ss.str();
}

time_t Acore::Time::GetNextTimeWithDayAndHour(int8 dayOfWeek, int8 hour)
{
    if (hour < 0 || hour > 23)
        hour = 0;

    tm localTm = TimeBreakdown();
    localTm.tm_hour = hour;
    localTm.tm_min = 0;
    localTm.tm_sec = 0;

    if (dayOfWeek < 0 || dayOfWeek > 6)
        dayOfWeek = (localTm.tm_wday + 1) % 7;

    uint32 add;
    if (localTm.tm_wday >= dayOfWeek)
        add = (7 - (localTm.tm_wday - dayOfWeek)) * DAY;
    else
        add = (dayOfWeek - localTm.tm_wday) * DAY;

    return mktime(&localTm) + add;
}

time_t Acore::Time::GetNextTimeWithMonthAndHour(int8 month, int8 hour)
{
    if (hour < 0 || hour > 23)
        hour = 0;

    tm localTm = TimeBreakdown();
    localTm.tm_mday = 1;
    localTm.tm_hour = hour;
    localTm.tm_min = 0;
    localTm.tm_sec = 0;

    if (month < 0 || month > 11)
    {
        month = (localTm.tm_mon + 1) % 12;

        if (!month)
            localTm.tm_year += 1;
    }
    else if (localTm.tm_mon >= month)
    {
        localTm.tm_year += 1;
    }

    localTm.tm_mon = month;
    return mktime(&localTm);
}

uint32 Acore::Time::GetSeconds(Seconds time /*= 0s*/)
{
    if (time == 0s)
        time = GetEpochTime();
    return TimeBreakdown(time.count()).tm_sec;
}

uint32 Acore::Time::GetMinutes(Seconds time /*= 0s*/)
{
    if (time == 0s)
        time = GetEpochTime();
    return TimeBreakdown(time.count()).tm_min;
}

uint32 Acore::Time::GetHours(Seconds time /*= 0s*/)
{
    if (time == 0s)
        time = GetEpochTime();
    return TimeBreakdown(time.count()).tm_hour;
}

uint32 Acore::Time::GetDayInWeek(Seconds time /*= 0s*/)
{
    if (time == 0s)
        time = GetEpochTime();
    return TimeBreakdown(time.count()).tm_wday;
}

uint32 Acore::Time::GetDayInMonth(Seconds time /*= 0s*/)
{
    if (time == 0s)
        time = GetEpochTime();
    return TimeBreakdown(time.count()).tm_mday;
}

uint32 Acore::Time::GetDayInYear(Seconds time /*= 0s*/)
{
    if (time == 0s)
        time = GetEpochTime();
    return TimeBreakdown(time.count()).tm_yday;
}

uint32 Acore::Time::GetMonth(Seconds time /*= 0s*/)
{
    if (time == 0s)
        time = GetEpochTime();
    return TimeBreakdown(time.count()).tm_mon;
}

uint32 Acore::Time::GetYear(Seconds time /*= 0s*/)
{
    if (time == 0s)
        time = GetEpochTime();
    return TimeBreakdown(time.count()).tm_year;
}
