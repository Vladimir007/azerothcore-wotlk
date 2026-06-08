#ifndef NCORE_CRYPTO_RANDOM_H
#define NCORE_CRYPTO_RANDOM_H

#include <array>
#include <openssl/rand.h>
#include "Define.h"
#include "Errors.h"

namespace Acore::Crypto
{
    inline void GetRandomBytes(uint8* buf, const std::size_t len)
    {
        const int result = RAND_bytes(buf, len);
        ASSERT(result == 1, "Not enough randomness in OpenSSL's entropy pool. What in the world are you running on?");
    }

    template <typename Container>
    void GetRandomBytes(Container& c)
    {
        GetRandomBytes(std::data(c), std::size(c));
    }

    template <std::size_t S>
    std::array<uint8, S> GetRandomBytes()
    {
        std::array<uint8, S> arr;
        GetRandomBytes(arr);
        return arr;
    }
}

#endif
