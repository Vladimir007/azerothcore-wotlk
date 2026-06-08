#ifndef NCORE_TIMER_H
#define NCORE_TIMER_H

#include "Common.h"
#include "Duration.h"

enum class TimeFormat : uint8
{
    FullText,  // 1 Days 2 Hours 3 Minutes 4 Seconds 5 Milliseconds
    ShortText, // 1d 2h 3m 4s 5ms
    Numeric    // 1:2:3:4:5
};

enum class TimeOutput : uint8
{
    Days,         // 1d
    Hours,        // 1d 2h
    Minutes,      // 1d 2h 3m
    Seconds,      // 1d 2h 3m 4s
    Milliseconds, // 1d 2h 3m 4s 5ms
    Microseconds  // 1d 2h 3m 4s 5ms 6us
};

namespace Acore::Time
{
    std::tm TimeBreakdown(time_t t = 0);
    std::string TimeToTimestampStr(Seconds time = 0s, std::string_view fmt = {});
    std::string TimeToHumanReadable(Seconds time = 0s, std::string_view fmt = {});

    time_t GetNextTimeWithDayAndHour(int8 dayOfWeek, int8 hour); // int8 dayOfWeek: 0 (sunday) to 6 (saturday)
    time_t GetNextTimeWithMonthAndHour(int8 month, int8 hour); // int8 month: 0 (january) to 11 (december)

    uint32 GetSeconds(Seconds time = 0s);      // Seconds after the minute - [0, 60]
    uint32 GetMinutes(Seconds time = 0s);      // Minutes after the hour - [0, 59]
    uint32 GetHours(Seconds time = 0s);        // Hours since midnight - [0, 23]
    uint32 GetDayInWeek(Seconds time = 0s);    // Days since Sunday - [0, 6]
    uint32 GetDayInMonth(Seconds time = 0s);   // Day of the month - [1, 31]
    uint32 GetDayInYear(Seconds time = 0s);    // Days since January 1 - [0, 365]
    uint32 GetMonth(Seconds time = 0s);        // Months since January - [0, 11]
    uint32 GetYear(Seconds time = 0s);         // Years since 1900
}

tm* localtime_r(time_t const* __timer, tm* __tp);

inline TimePoint GetApplicationStartTime()
{
    using namespace std::chrono;

    static const TimePoint ApplicationStartTime = steady_clock::now();
    return ApplicationStartTime;
}

inline Milliseconds GetTimeMS()
{
    using namespace std::chrono;

    return duration_cast<milliseconds>(steady_clock::now() - GetApplicationStartTime());
}

inline Milliseconds GetMSTimeDiff(const Milliseconds oldMSTime, const Milliseconds newMSTime)
{
    if (oldMSTime > newMSTime)
        return oldMSTime - newMSTime;
    return newMSTime - oldMSTime;
}

inline uint32 getMSTime()
{
    using namespace std::chrono;
    return static_cast<uint32>(duration_cast<milliseconds>(steady_clock::now() - GetApplicationStartTime()).count());
}

inline uint32 getMSTimeDiff(const uint32 oldMSTime, const uint32 newMSTime)
{
    // getMSTime() have limited data range and this is case when it overflows in this tick
    if (oldMSTime > newMSTime)
        return 0xFFFFFFFF - oldMSTime + newMSTime;
    return newMSTime - oldMSTime;
}

inline uint32 getMSTimeDiff(const uint32 oldMSTime, const TimePoint newTime)
{
    using namespace std::chrono;

    const uint32 newMSTime = static_cast<uint32>(duration_cast<milliseconds>(newTime - GetApplicationStartTime()).count());
    return getMSTimeDiff(oldMSTime, newMSTime);
}

inline uint32 GetMSTimeDiffToNow(const uint32 oldMSTime)
{
    return getMSTimeDiff(oldMSTime, getMSTime());
}

inline Milliseconds GetMSTimeDiffToNow(const Milliseconds oldMSTime)
{
    return GetMSTimeDiff(oldMSTime, GetTimeMS());
}

inline Seconds GetEpochTime()
{
    using namespace std::chrono;
    return duration_cast<Seconds>(system_clock::now().time_since_epoch());
}

struct IntervalTimer
{
    IntervalTimer() = default;

    void Update(const time_t diff)
    {
        _current += diff;
        if (_current < 0)
            _current = 0;
    }

    void Reset()
    {
        if (_current >= _interval)
            _current %= _interval;
    }

    void SetCurrent(const time_t current) { _current = current; }
    void SetInterval(const time_t interval) { _interval = interval; }

    [[nodiscard]] bool Passed() const { return _current >= _interval; }
    [[nodiscard]] time_t GetInterval() const { return _interval; }
    [[nodiscard]] time_t GetCurrent() const { return _current; }

private:
    time_t _interval{0};
    time_t _current{0};
};

struct TimeTracker
{
    explicit TimeTracker(const time_t expiry) : i_expiryTime(expiry) {}

    void Update(const time_t diff) { i_expiryTime -= diff; }
    void Reset(const time_t interval) { i_expiryTime = interval; }

    [[nodiscard]] bool Passed() const { return i_expiryTime <= 0; }
    [[nodiscard]] time_t GetExpiry() const { return i_expiryTime; }

private:
    time_t i_expiryTime;
};

struct TimeTrackerSmall
{
    explicit TimeTrackerSmall(const int32 expiry = 0) : i_expiryTime(expiry) {}

    void Update(const int32 diff) { i_expiryTime -= diff; }
    void Reset(const int32 interval) { i_expiryTime = interval; }

    [[nodiscard]] bool Passed() const { return i_expiryTime <= 0; }
    [[nodiscard]] int32 GetExpiry() const { return i_expiryTime; }

private:
    int32 i_expiryTime;
};

#endif
