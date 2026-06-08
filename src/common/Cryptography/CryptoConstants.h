#ifndef NCORE_CRYPTO_CONSTANTS_H
#define NCORE_CRYPTO_CONSTANTS_H

namespace Acore::Crypto
{
    struct Constants
    {
        static constexpr std::size_t MD5_DIGEST_LENGTH_BYTES = 16;
        static constexpr std::size_t SHA1_DIGEST_LENGTH_BYTES = 20;
        static constexpr std::size_t SHA256_DIGEST_LENGTH_BYTES = 32;
    };
}

#endif
