#ifndef DURATION_H
#define DURATION_H

#include <chrono>

/// Microseconds shorthand typedef.
using Microseconds = std::chrono::microseconds;

/// Milliseconds shorthand typedef.
using Milliseconds = std::chrono::milliseconds;

/// Seconds shorthand typedef.
using Seconds = std::chrono::seconds;

/// Minutes shorthand typedef.
using Minutes = std::chrono::minutes;

/// Hours shorthand typedef.
using Hours = std::chrono::hours;

// Workaround for GCC and Clang 10 in C++20
#if defined(__GNUC__) && (!defined(__clang__) || (__clang_major__ == 10))
/// Days shorthand typedef.
using Days = std::chrono::duration<__INT64_TYPE__, std::ratio<86400>>;

/// Weeks shorthand typedef.
using Weeks = std::chrono::duration<__INT64_TYPE__, std::ratio<604800>>;

/// Years shorthand typedef.
using Years = std::chrono::duration<__INT64_TYPE__, std::ratio<31556952>>;

/// Months shorthand typedef.
using Months = std::chrono::duration<__INT64_TYPE__, std::ratio<2629746>>;

#else

/// Days shorthand typedef.
using Days = std::chrono::days;

/// Weeks shorthand typedef.
using Weeks = std::chrono::weeks;

/// Years shorthand typedef.
using Years = std::chrono::years;

/// Months shorthand typedef.
using Months = std::chrono::months;

#endif // GCC_VERSION

/// time_point shorthand typedefs
using TimePoint = std::chrono::steady_clock::time_point;
using SystemTimePoint = std::chrono::system_clock::time_point;

/// Makes std::chrono_literals globally available.
using namespace std::chrono_literals;

constexpr Days operator""_days(const unsigned long long days)
{
    return Days(days);
}

constexpr Weeks operator""_weeks(const unsigned long long weeks)
{
    return Weeks(weeks);
}

constexpr Years operator""_years(const unsigned long long years)
{
    return Years(years);
}

constexpr Months operator""_months(const unsigned long long months)
{
    return Months(months);
}

#endif
