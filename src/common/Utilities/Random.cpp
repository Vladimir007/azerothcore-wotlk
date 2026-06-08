#include "Random.h"

#include <memory>
#include <random>

#include "Errors.h"
#include "SFMTRand.h"

static thread_local std::unique_ptr<SFMTRand> sfmtRand;
static RandomEngine engine;

static SFMTRand* GetRng()
{
    if (!sfmtRand)
        sfmtRand = std::make_unique<SFMTRand>();

    return sfmtRand.get();
}

int32 irand(const int32 min, const int32 max)
{
    ASSERT(max >= min);
    std::uniform_int_distribution uid(min, max);
    return uid(engine);
}

uint32 urand(const uint32 min, const uint32 max)
{
    ASSERT(max >= min);
    std::uniform_int_distribution uid(min, max);
    return uid(engine);
}

uint32 urandms(const uint32 min, const uint32 max)
{
    ASSERT(std::numeric_limits<uint32>::max() / Milliseconds::period::den >= max);
    return urand(min * Milliseconds::period::den, max * Milliseconds::period::den);
}

float frand(const float min, const float max)
{
    ASSERT(max >= min);
    std::uniform_real_distribution urd(min, max);
    return urd(engine);
}

Milliseconds randtime(const Milliseconds min, const Milliseconds max)
{
    const long long diff = max.count() - min.count();
    ASSERT(diff >= 0);
    ASSERT(diff <= static_cast<uint32>(-1));
    return min + Milliseconds(urand(0, diff));
}

Seconds randtime(const Seconds min, const Seconds max)
{
    const long long diff = max.count() - min.count();
    ASSERT(diff >= 0);
    ASSERT(diff <= static_cast<uint32>(-1));
    return min + Seconds(urand(0, diff));
}

uint32 rand32()
{
    return GetRng()->RandomUInt32();
}

double rand_norm()
{
    std::uniform_real_distribution<> urd;
    return urd(engine);
}

double rand_chance()
{
    std::uniform_real_distribution urd(0.0, 100.0);
    return urd(engine);
}

uint32 urandweighted(const std::size_t count, double const* chances)
{
    std::discrete_distribution<uint32> dd(chances, chances + count);
    return dd(engine);
}

RandomEngine& RandomEngine::Instance()
{
    return engine;
}
