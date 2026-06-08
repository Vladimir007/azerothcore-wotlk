#include "ARC4.h"
#include "Errors.h"

Acore::Crypto::ARC4::ARC4() : _ctx(EVP_CIPHER_CTX_new())
{
    _cipher = EVP_CIPHER_fetch(nullptr, "RC4", nullptr);

    EVP_CIPHER_CTX_init(_ctx);
    const int result = EVP_EncryptInit_ex(_ctx, _cipher, nullptr, nullptr, nullptr);
    ASSERT(result == 1);
}

Acore::Crypto::ARC4::~ARC4()
{
    EVP_CIPHER_CTX_free(_ctx);
    EVP_CIPHER_free(_cipher);
}

void Acore::Crypto::ARC4::Init(uint8 const* seed, const std::size_t len)
{
    const int result1 = EVP_CIPHER_CTX_set_key_length(_ctx, len);
    ASSERT(result1 == 1);
    const int result2 = EVP_EncryptInit_ex(_ctx, nullptr, nullptr, seed, nullptr);
    ASSERT(result2 == 1);
}

void Acore::Crypto::ARC4::UpdateData(uint8* data, const std::size_t len)
{
    int outLen = 0;
    const int result1 = EVP_EncryptUpdate(_ctx, data, &outLen, data, len);
    ASSERT(result1 == 1);
    const int result2 = EVP_EncryptFinal_ex(_ctx, data, &outLen);
    ASSERT(result2 == 1);
}
