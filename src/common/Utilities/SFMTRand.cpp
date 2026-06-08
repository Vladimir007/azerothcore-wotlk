#include "SFMTRand.h"

#include <algorithm>
#include <array>
#include <emmintrin.h>
#include <random>

#include "Timer.h"

SFMTRand::SFMTRand()
{
    if (std::random_device dev; dev.entropy() > 0)
    {
        std::array<uint32, SFMT_N32> seed;
        std::ranges::generate(seed, std::ref(dev));
        sfmt_init_by_array(&_state, seed.data(), seed.size());
    }
    else
        sfmt_init_gen_rand(&_state, static_cast<uint32>(GetEpochTime().count()));
}

uint32 SFMTRand::RandomUInt32()
{
    return sfmt_genrand_uint32(&_state);
}

void* SFMTRand::operator new(const std::size_t size, std::nothrow_t const&)
{
    return _mm_malloc(size, 16);
}

void SFMTRand::operator delete(void* ptr, std::nothrow_t const&)
{
    _mm_free(ptr);
}

void* SFMTRand::operator new(const std::size_t size)
{
    return _mm_malloc(size, 16);
}

void SFMTRand::operator delete(void* ptr)
{
    _mm_free(ptr);
}

void* SFMTRand::operator new[](const std::size_t size, std::nothrow_t const&)
{
    return _mm_malloc(size, 16);
}

void SFMTRand::operator delete[](void* ptr, std::nothrow_t const&)
{
    _mm_free(ptr);
}

void* SFMTRand::operator new[](const std::size_t size)
{
    return _mm_malloc(size, 16);
}

void SFMTRand::operator delete[](void* ptr)
{
    _mm_free(ptr);
}
