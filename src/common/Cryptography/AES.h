#ifndef NCORE_AES_H
#define NCORE_AES_H

#include <array>
#include <openssl/evp.h>
#include "Define.h"

namespace Acore::Crypto
{
    class AES
    {
    public:
        static constexpr std::size_t IV_SIZE_BYTES = 12;
        static constexpr std::size_t KEY_SIZE_BYTES = 16;
        static constexpr std::size_t TAG_SIZE_BYTES = 12;

        using IV = std::array<uint8, IV_SIZE_BYTES>;
        using Key = std::array<uint8, KEY_SIZE_BYTES>;
        using Tag = uint8[TAG_SIZE_BYTES];

        explicit AES(bool encrypting);
        ~AES();

        void Init(Key const& key);

        bool Process(IV const& iv, uint8* data, std::size_t length, Tag& tag);

    private:
        EVP_CIPHER_CTX* _ctx;
        bool _encrypting;
    };
}

#endif
