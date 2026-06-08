#ifndef NCORE_ARC4_H
#define NCORE_ARC4_H

#include <openssl/evp.h>
#include "Define.h"

namespace Acore::Crypto
{
    class ARC4
    {
    public:
        ARC4();
        ~ARC4();

        void Init(uint8 const* seed, std::size_t len);

        template <typename Container>
        void Init(Container const& c) { Init(std::data(c), std::size(c)); }

        void UpdateData(uint8* data, std::size_t len);

        template <typename Container>
        void UpdateData(Container& c) { UpdateData(std::data(c), std::size(c)); }
    private:
        EVP_CIPHER* _cipher;
        EVP_CIPHER_CTX* _ctx;
    };
}

#endif
