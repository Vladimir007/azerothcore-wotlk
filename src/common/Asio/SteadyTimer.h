#ifndef STEADY_TIMER_HPP
#define STEADY_TIMER_HPP

#include <chrono>
#include "Define.h"

namespace Acore::Asio::SteadyTimer
{
    inline auto GetExpirationTime(const int32 seconds)
    {
        return std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    }
}

#endif
